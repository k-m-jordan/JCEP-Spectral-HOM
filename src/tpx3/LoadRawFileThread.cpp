#include "tpx3/tpx3.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <vector>
#include <set>
#include <cmath>

#include <tim/timsort.h>

#define PCL_NO_PRECOMPILE
#include <pcl/point_cloud.h>
#include <pcl/octree/octree_search.h>

using OctreePoint = pcl::PointXYZ;

using namespace spec_hom;

void sort_timestamps(PixelData &data) {

    auto num_packets = data.addr.size();

    std::vector<std::size_t> indices(num_packets);
    std::iota(indices.begin(), indices.end(), 0); // fill with index values

    auto &timestamps = data.toa;

    // sort indices based on timestamp values
    tim::timsort(indices.begin(), indices.end(), [&timestamps] (std::size_t i1, std::size_t i2) { return timestamps[i1] < timestamps[i2]; });

    // now indices is sorted properly, and we need to create sorted address and toa arrays
    std::vector<PixelAddr> sorted_addr(num_packets);
    std::vector<int64_t> sorted_toa(num_packets);
    std::vector<uint16_t> sorted_tot(num_packets);

    for(std::size_t i = 0; i < num_packets; ++i) {
        auto ix = indices[i];
        sorted_addr[i] = data.addr[ix];
        sorted_toa[i] = data.toa[ix];
        sorted_tot[i] = data.tot[ix];
    }

    data.addr.swap(sorted_addr);
    data.toa.swap(sorted_toa);
    data.tot.swap(sorted_tot);

}

LoadRawFileThread::LoadRawFileThread(const std::string &fname, Tpx3ImportSettings settings, bool raw_packets_only) :
    BgThread(),
    mFileName(fname),
    mImportSettings(settings),
    mRawPacketsOnly(raw_packets_only) {

    // Do nothing

}

PixelData LoadRawFileThread::parseRawData() {

    std::vector<PixelAddr> addr_vec;
    std::vector<int64_t> toa_vec;
    std::vector<uint16_t> tot_vec;

    auto mask = mImportSettings.spatialMask;

    std::ifstream data_stream(mFileName, std::ios::binary);

    constexpr unsigned SIZE_OF_PACKET = 8; // in bytes

    // get file size
    data_stream.seekg(0, std::ios::end);
    unsigned file_size = data_stream.tellg();
    data_stream.seekg(0, std::ios::beg);

    // prepare enough memory to read the max number of packets possible
    addr_vec.reserve((file_size + SIZE_OF_PACKET - 1) / SIZE_OF_PACKET);
    toa_vec.reserve((file_size + SIZE_OF_PACKET - 1) / SIZE_OF_PACKET);
    tot_vec.reserve((file_size + SIZE_OF_PACKET - 1) / SIZE_OF_PACKET);

    while(data_stream) {

        // at the start of a chunk
        uint8_t chunk_header[8]; // headers are 8 bytes

        // check that we have data remaining
        data_stream.read(reinterpret_cast<char *>(chunk_header), 1);
        if (!data_stream) {
            // out of data - continue with processing
            break;
        }

        data_stream.read(reinterpret_cast<char *>(chunk_header + 1), sizeof(chunk_header) - 1);
        if (!data_stream) {
            emit err("Failed to load file: incomplete chunk header");
            return {};
        }
        if (!(chunk_header[0] == 'T'
              && chunk_header[1] == 'P'
              && chunk_header[2] == 'X'
              && chunk_header[3] == '3')) {
            emit err("Failed to load file: corrupt chunk header");
            return {};
        }

        // chunk_header[4]: chip index
        assert(chunk_header[4] == 0); // reading from multiple chips is not currently supported
        // chunk_header[5]: unused

        uint16_t chunk_size = (static_cast<uint16_t>(chunk_header[7]) << 8) + chunk_header[6];
        if (chunk_size % 8) {
            emit err("Failed to load file: corrupt chunk header");
            return {};
        }
        chunk_size /= 8;

        for (unsigned chunk_ix = 0; chunk_ix < chunk_size; ++chunk_ix) {

            // read a packet
            uint8_t packet[SIZE_OF_PACKET];
            data_stream.read(reinterpret_cast<char *>(packet), sizeof(packet));

            uint8_t packet_header = (((packet[7]) & 0xF0) >> 4);
            switch (packet_header) {
                case 0xb: {
                    // pixel data should always be little endian
                    uint64_t full_data = (packet[0])
                                         | (static_cast<uint64_t>(packet[1]) << 8)
                                         | (static_cast<uint64_t>(packet[2]) << 16)
                                         | (static_cast<uint64_t>(packet[3]) << 24)
                                         | (static_cast<uint64_t>(packet[4]) << 32)
                                         | (static_cast<uint64_t>(packet[5]) << 40)
                                         | (static_cast<uint64_t>(packet[6]) << 48)
                                         | (static_cast<uint64_t>(packet[7]) << 56);

                    // pixel address in super-pixel coordinates
                    uint16_t addr            = static_cast<uint16_t>((full_data & 0x0FFFF00000000000) >> 44);
                    // fine time of arrival (640 MHz clock)
                    uint8_t chip_fine_toa    = static_cast<uint8_t> ((full_data & 0x00000000000F0000) >> 16);
                    // coarse time of arrival (40 MHz clock)
                    uint16_t chip_coarse_toa = static_cast<uint16_t>((full_data & 0x00000FFFC0000000) >> 30);
                    // SPIDR time (40 MHz clock, units of 2^14 ticks)
                    uint16_t spidr_toa       = static_cast<uint16_t> (full_data & 0x000000000000FFFF);
                    // time over threshold (40 MHz clock)
                    uint16_t tot             = static_cast<uint16_t>((full_data & 0x000000003FF00000) >> 20);

                    // combine coarse & SPIDR times
                    uint32_t combined_coarse = (static_cast<uint32_t>(spidr_toa) << 14) | chip_coarse_toa;

                    PixelAddr addr_2d {
                            static_cast<uint8_t>(((addr >> 1) & 0x00FC) | (addr & 0x0003)),
                            static_cast<uint8_t>(((addr >> 8) & 0xFE) | ((addr >> 2) & 0x0001))
                    };

                    addr_2d.y = TPX3_SENSOR_SIZE - 1 - addr_2d.y; // flip y direction

                    uint8_t mask_ix;
                    if(mask.vertical)
                        mask_ix = addr_2d.x;
                    else
                        mask_ix = addr_2d.y;

                    if(!((mask_ix < mask.max1 && mask_ix > mask.min1) || (mask_ix < mask.max2 && mask_ix > mask.min2))) {
                        continue;
                    }

                    chip_fine_toa = chip_fine_toa ^ 0x0F; // fine toa counts backwards

                    // convert address to XY coordinates
                    addr_vec.push_back(addr_2d);
                    // combined ToA
                    int64_t toa = static_cast<int64_t>((static_cast<uint64_t>(combined_coarse) << 4) | chip_fine_toa);

                    if(tot < mImportSettings.totCorrection.size())
                        toa += static_cast<int>(std::round(mImportSettings.totCorrection[tot]/MIN_TICK));

                    toa_vec.push_back(
                        toa
                    );

                    tot_vec.push_back(
                            tot // time over threshold
                    );

                } break;
                case 0x6:
                    emit warn("Chunk header 0x6 (TDC counter) is not implemented");
                    return {};
                case 0x4:
                    emit warn("Chunk header 0x4 (software timestamp) is not implemented");
                    return {};
                case 0x7:
                    // control, ignore
                    break;
                default:
                    emit warn("Unknown packet header: " + std::to_string(packet_header));
                    return {};

            }

        }

        emit setProgress(static_cast<int>(static_cast<float>(data_stream.tellg()) / file_size * 100));

        if(shouldCancel()) {
            return {};
        }

    }

    // free unused memory
    addr_vec.shrink_to_fit();
    toa_vec.shrink_to_fit();
    tot_vec.shrink_to_fit();

    return PixelData {
            std::move(addr_vec),
            std::move(toa_vec),
            std::move(tot_vec)
    };

}

ClusterData LoadRawFileThread::cluster(const PixelData &raw_data) {

    const float SPACE_WINDOW = mImportSettings.clusterSizeXY * 2;
    const float TIME_WINDOW = mImportSettings.clusterSizeT / 1.5625f * 2;
    const int MIN_CLUSTER_SIZE = mImportSettings.minClusterSize;

    auto time_half_window = TIME_WINDOW / 2.0f;
    auto space_half_window = SPACE_WINDOW / 2.0f;

    emit setProgress(0);
    emit setProgressText("Preparing to cluster... (%p%)");
    emit setProgressIndefinite(false);

    std::size_t num_raw_packets = raw_data.numPackets();
    std::size_t one_percent_packets = num_raw_packets / 100 + 1; // extra one is to avoid missing packets due to divide's truncation

    auto toa_max = raw_data.toa.back(), toa_min = raw_data.toa.front();
    auto toa_mid = (toa_max + toa_min) / 2.0f, toa_half_range = (toa_max - toa_min) / 2.0f;

    auto space_max = 128/space_half_window, space_min = -128/space_half_window;
    auto time_max = (toa_max - toa_mid) / time_half_window, time_min = (toa_min - toa_mid) / time_half_window;

    auto scaled_space_half_window = 1;
    auto scaled_time_half_window = 1;

    pcl::PointCloud<OctreePoint>::Ptr point_cloud(new pcl::PointCloud<OctreePoint>);

    point_cloud->width = num_raw_packets;
    point_cloud->height = 1;
    point_cloud->points.reserve(point_cloud->width * point_cloud->height);

    // using the space/time window as the octree's voxel resolution means that voxel-based errors don't affect the clusters
    auto resolution = std::min(scaled_space_half_window, scaled_time_half_window);
    pcl::octree::OctreePointCloudSearch<OctreePoint> octree(resolution);
    octree.setInputCloud(point_cloud);
    octree.defineBoundingBox(space_min, space_min, time_min, space_max, space_max, time_max);

    for(int percent = 0; percent < 100; ++percent) {
        for (std::size_t ix = percent*one_percent_packets; ix < ((percent+1)*one_percent_packets) && (ix < num_raw_packets); ++ix) {
            octree.addPointToCloud({
                    static_cast<float>(raw_data.addr[ix].x - 128) / space_half_window,
                    static_cast<float>(raw_data.addr[ix].y - 128) / space_half_window,
                    static_cast<float>(raw_data.toa[ix] - toa_mid) / time_half_window
            }, point_cloud);
        }
        emit setProgress(percent);
        if(shouldCancel())
            return {};
    }

    emit setProgressText("Preparing clustering tree...");
    emit setProgressIndefinite(true);

    std::vector<int> packet_clusters(num_raw_packets, -1); // stores the cluster number of each raw packet
    std::size_t earliest_remaining_packet = 0;

    std::set<std::size_t> cluster_indices, old_cluster_indices; // set to store the indices of our cluster (this way we don't have to worry about duplicate inclusions
    std::vector<std::size_t> new_cluster_indices, prev_cluster_indices; // vector to store recently found indices

    std::vector<int> octree_search_indices;

    emit setProgressText("Clustering... (%p%)");
    emit setProgressIndefinite(false);
    emit setProgress(0);

    std::size_t clustered_packets = 0;

    unsigned iteration = 0;
    int current_cluster = 1;

    // while we still have points
    while(earliest_remaining_packet < num_raw_packets) {

        // we can skip any packets that have already been clustered
        // since clusters are independent of starting cluster, if one cluster is not clustered, we can be guaranteed that
        // all packets in the cluster are not marked yet
        if(packet_clusters[earliest_remaining_packet] < 0) {
            cluster_indices.clear();
            new_cluster_indices.clear();

            // by definition, this point will be in a cluster
            cluster_indices.insert(earliest_remaining_packet);
            new_cluster_indices.push_back(earliest_remaining_packet);

            bool found_new_indices = true;

            // keep looking for new points until we stop finding them
            while(found_new_indices) {
                found_new_indices = false;

                prev_cluster_indices.clear();
                new_cluster_indices.swap(prev_cluster_indices);

                for(auto jx : prev_cluster_indices) {
                    if(packet_clusters[jx] >= 0)
                        continue;

                    octree_search_indices.clear();

                    auto &jx_pt = (*point_cloud)[jx];

                    // unfortunate and unavoidable cast to single-precision float :(
                    Eigen::Vector3f upper_bound {
                            jx_pt.x + scaled_space_half_window,
                            jx_pt.y + scaled_space_half_window,
                            jx_pt.z + scaled_time_half_window,
                    };
                    Eigen::Vector3f lower_bound {
                            jx_pt.x - scaled_space_half_window,
                            jx_pt.y - scaled_space_half_window,
                            jx_pt.z - scaled_time_half_window,
                    };

                    if(octree.boxSearch(lower_bound, upper_bound, octree_search_indices)) {
                        old_cluster_indices = cluster_indices;
                        cluster_indices.insert(octree_search_indices.begin(), octree_search_indices.end());

                        octree_search_indices.clear();

                        std::set_difference(cluster_indices.begin(), cluster_indices.end(),
                                            old_cluster_indices.begin(), old_cluster_indices.end(),
                                            std::back_inserter(new_cluster_indices));
                    }

                }

                found_new_indices = new_cluster_indices.size();
            }

            if(shouldCancel())
                return {};

            clustered_packets += cluster_indices.size();

            ++iteration;
            if((iteration % 1000 )== 0)
                emit setProgress(static_cast<int>(100.0 * static_cast<float>(clustered_packets)/num_raw_packets));

            if(cluster_indices.size() < MIN_CLUSTER_SIZE) {
                for(auto jx : cluster_indices)
                    packet_clusters[jx] = 0;
                continue;
            }

            bool nonempty = false;
            for(auto jx : cluster_indices) {
                if(packet_clusters[jx] == -1) {
                    packet_clusters[jx] = current_cluster;
                    nonempty = true;
                }
            }

            if(nonempty)
                ++current_cluster;

        }

        ++earliest_remaining_packet;

    }

    emit setProgress(0);
    emit setProgressText("Done clustering...");
    emit setProgressIndefinite(true);

    return {
        current_cluster - 1, // important correction
        std::move(packet_clusters)
    };

}

std::vector<ClusterCentroid> LoadRawFileThread::centroid(const PixelData &data, const ClusterData &clusters) {

    // array to hold centroided xyt positions
    std::vector<ClusterCentroid> events;
    events.resize(clusters.num_clusters);

    std::vector<double> cluster_x_weighted_sums(clusters.num_clusters, 0);
    std::vector<double> cluster_y_weighted_sums(clusters.num_clusters, 0);
    std::vector<uint16_t> cluster_total_weights(clusters.num_clusters, 0);
    std::vector<uint16_t> cluster_max_tot(clusters.num_clusters, 0);
    std::vector<double> cluster_toa(clusters.num_clusters, 0);

    // briefly: The brightest pixel (largest ToT) is used to find the time. This is because lower-ToT pixels have a slower rise time, and so a later ToA.
    // The positions of all cluster pixels are centroided to find location, with the weighting function being the ToT (roughly, the energy) of each pixel
    emit setProgressText("Centroiding... (%p%)");
    emit setProgress(0);
    emit setProgressIndefinite(false);

    // recall that clusters start at index 1 (index 0 = unclustered packets)
    for(auto ix = 0; ix < data.numPackets(); ++ix) {
        if(clusters.cluster_ids[ix] == 0)
            continue;

        auto cluster_id = clusters.cluster_ids[ix] - 1;
        auto x = static_cast<double>(data.addr[ix].x)*PIXEL_SIZE; // [m]
        auto y = static_cast<double>(data.addr[ix].y)*PIXEL_SIZE; // [m]
        auto toa = static_cast<double>(data.toa[ix])*1.5625e-9; // [ns]
        auto tot = data.tot[ix];

        auto weight = tot;

        cluster_x_weighted_sums[cluster_id] += x*weight;
        cluster_y_weighted_sums[cluster_id] += y*weight;

        cluster_total_weights[cluster_id] += weight;

        if(tot > cluster_max_tot[cluster_id]) {
            cluster_max_tot[cluster_id] = tot;
            cluster_toa[cluster_id] = toa;
        }

        if((ix % (data.numPackets()/100)) == 0)
            emit setProgress(static_cast<int>(100*static_cast<double>(ix)/data.numPackets()));

        if(shouldCancel())
            return {};
    }

    for(auto ix = 0; ix < clusters.num_clusters; ++ix) {
        assert(cluster_total_weights[ix]);
        events[ix] = {
                cluster_x_weighted_sums[ix] / cluster_total_weights[ix],
                cluster_y_weighted_sums[ix] / cluster_total_weights[ix],
                cluster_toa[ix]
        };
    }

    return events;

}

std::pair<std::vector<CoincidencePair>, std::vector<CoincidenceNFold>> LoadRawFileThread::findCoincidences(const ClusterData &clusters, const std::vector<ClusterCentroid> &centroids) {

    std::vector<CoincidencePair> coinc_pairs;
    std::vector<CoincidenceNFold> coinc_nfolds;

    double window_size = mImportSettings.coincidenceWindow;

    // we first need a time-ordered list of all centroids
    std::vector<unsigned> sorted_indices;
    sorted_indices.reserve(clusters.num_clusters);
    for(auto ix = 0; ix < clusters.num_clusters; ++ix)
        sorted_indices.push_back(ix);

    emit setProgressText("Sorting centroids...");
    emit setProgressIndefinite(true);

    std::sort(sorted_indices.begin(), sorted_indices.end(), [&centroids](auto lhs, auto rhs){
        return centroids[lhs].toa < centroids[rhs].toa;
    });

    emit setProgressText("Finding coincidences...");
    emit setProgress(0);
    emit setProgressIndefinite(false);

    unsigned one_percent_clusters = clusters.num_clusters / 100 + 1;
    int last_progress = 0;

    std::vector<unsigned> curr_coinc;

    for(auto ix = 0; ix < clusters.num_clusters; ++ix) {
        auto progress = static_cast<int>(static_cast<double>(ix) / one_percent_clusters);
        if(progress != last_progress) {
            setProgress(progress);
            last_progress = progress;
        }
        if(shouldCancel())
            return {};

        auto toa = centroids[sorted_indices[ix]].toa;

        // difference between current toa and ix's toa
        bool within_window = true;
        unsigned offset = 1;
        while(within_window) {
            if(ix+offset >= clusters.num_clusters)
                break;

            auto curr_toa = centroids[sorted_indices[ix + offset]].toa;
            auto dtoa = curr_toa - toa;

            if(dtoa <= window_size) {
                if(offset == 1) // only two coincidences so far
                    curr_coinc.push_back(sorted_indices[ix]); // add the first index
                curr_coinc.push_back(sorted_indices[ix+offset]); // add the second (third, fourth...) index
                ++offset;
            } else {
                within_window = false;
            }
        }

        if(offset == 1) {
            // no coincidence found within window
        } else {
            if(curr_coinc.size() == 2) {
                coinc_pairs.push_back({
                    curr_coinc[0],
                    curr_coinc[1]
                });
            } else {
                coinc_nfolds.emplace_back(curr_coinc);
            }
            curr_coinc.clear();

            ix += offset - 1; // skip any clusters already processed
        }
    }

    std::cout << "Found " << coinc_pairs.size() << " pairs" << std::endl;
    std::cout << "Found " << coinc_nfolds.size() << " n-fold coincidences" << std::endl;

    return std::make_pair(std::move(coinc_pairs), std::move(coinc_nfolds));

}

void LoadRawFileThread::execute() {

    emit setProgress(0);
    emit setProgressText("Loading raw Tpx3 data... (%p%)");
    emit setProgressTextColor(QColor(0,0,0));

    PixelData data = parseRawData();
    if(shouldCancel()) { // either an error, or thread was cancelled
        finish();
        return;
    }

    if(mRawPacketsOnly) {
        finish(std::move(data), {}, {}, {}, {});
        return;
    }

    if(data.isEmpty()) {
        emit warn("No raw packets found within mask.");
        finish();
        return;
    }

    emit setProgressText("Sorting timestamp data...");
    emit setProgressIndefinite(true); // switch to an indefinite progress bar
    sort_timestamps(data);

    // emit a warning
    bool zero_tot_corr = true;
    for(auto x : mImportSettings.totCorrection)
        zero_tot_corr &= (x == 0);
    if(mImportSettings.minClusterSize < 3 && zero_tot_corr)
        emit warn("Low cluster size, and no ToA calibration set - may be missing some coincidences.");

    ClusterData clusters = cluster(data);
    if(shouldCancel()) {
        finish();
        return;
    }

    std::vector<ClusterCentroid> centroids = centroid(data, clusters);
    if(shouldCancel()) {
        finish();
        return;
    }

    std::vector<CoincidencePair> coinc_pairs;
    std::vector<CoincidenceNFold> coinc_nfolds;
    std::tie(coinc_pairs, coinc_nfolds) = findCoincidences(clusters, centroids);
    if(shouldCancel()) {
        finish();
        return;
    }

    finish(std::move(data), std::move(clusters), std::move(centroids), std::move(coinc_pairs), std::move(coinc_nfolds));

}

void LoadRawFileThread::finish() {

    finish({}, {}, {}, {}, {});

}

void LoadRawFileThread::finish(PixelData &&data, ClusterData &&clusters, std::vector<ClusterCentroid> &&centroids,
                               std::vector<CoincidencePair> &&coinc_pairs, std::vector<CoincidenceNFold> &&coinc_nfolds) {

    // indicates an error with the loading function
    assert((data.addr.size() == data.tot.size()) && (data.addr.size() == data.toa.size()));

    emit setProgressText("Post-processing...");
    emit setProgressIndefinite(true);

    std::unique_ptr<Tpx3Image> image = std::make_unique<Tpx3Image>(mFileName, std::move(data), std::move(clusters),
                                                                   std::move(centroids), std::move(coinc_pairs), std::move(coinc_nfolds),
                                                                   mImportSettings.calibration);

    emit yieldPixelData(image.release());

}