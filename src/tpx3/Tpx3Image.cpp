#include "tpx3.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>

using namespace spec_hom;

Tpx3Image::Tpx3Image(std::string fname, PixelData &&raw_data, ClusterData &&clusters,
                     std::vector<ClusterCentroid> &&centroids, std::vector<CoincidencePair> &&coinc_pairs,
                     std::vector<CoincidenceNFold> &&coinc_nfolds) :
        mFileName(std::move(fname)),
        mRawData(std::move(raw_data)),
        mClusters(std::move(clusters)),
        mCentroids(std::move(centroids)),
        mCoincidencePairs(std::move(coinc_pairs)),
        mCoincidenceNFold(std::move(coinc_nfolds)),
        mBiphotonClicks() {

    initializeSpectrum();

}

std::string Tpx3Image::filename() const {

    std::filesystem::path path(mFileName);
    return path.filename().string();

}

const std::string& Tpx3Image::fullFilename() const {

    return mFileName;

}

PixelData& Tpx3Image::data() {

    return mRawData;

}

const PixelData& Tpx3Image::data() const {

    return mRawData;

}

unsigned long Tpx3Image::numRawPackets() const {

    return data().numPackets();

}

unsigned long Tpx3Image::numClusters() const {

    return mClusters.num_clusters;

}

bool Tpx3Image::empty() const {

    return data().isEmpty();

}

ImageXY<unsigned> Tpx3Image::rawPacketImage() const {

    std::vector<unsigned> r;
    r.insert(r.begin(), TPX3_SENSOR_SIZE, 0);

    ImageXY<unsigned> pixel_counts;
    pixel_counts.insert(pixel_counts.begin(), TPX3_SENSOR_SIZE, r);

    for(auto &packet : mRawData.addr)
        ++pixel_counts[packet.x][packet.y];

    return pixel_counts;

}

std::pair<QVector<double>, QVector<double>> Tpx3Image::toTDistribution(unsigned int hist_bin_size) const {

    constexpr double TOT_UNIT_SIZE = 25e-9; // data in units of 25 ns

    unsigned num_packets = numRawPackets();

    unsigned tot_hist[1024];
    for(unsigned ix = 0; ix < 1024; ++ix)
        tot_hist[ix] = 0;
    for(unsigned ix = 0; ix < num_packets; ++ix) {
        auto tot = data().tot[ix];
        ++tot_hist[tot];
    }

    auto hist_size = static_cast<unsigned>(std::ceil(static_cast<float>(1024) / hist_bin_size));

    QVector<double> x(hist_size), y(hist_size);
    for (int i=0; i<hist_size; ++i) {
        x[i] = i*hist_bin_size * TOT_UNIT_SIZE;
        y[i] = 0;
        for(auto j = i*hist_bin_size; (j < (i+1) * hist_bin_size) && (j < 1024); ++j)
            y[i] += tot_hist[j];
    }

    return std::make_pair<QVector<double>, QVector<double>>(std::move(x), std::move(y));

}

ImageXY<unsigned> Tpx3Image::clusterImage() const {

    std::vector<unsigned> r;
    r.insert(r.begin(), TPX3_SENSOR_SIZE, 0);

    ImageXY<unsigned> pixel_counts;
    pixel_counts.insert(pixel_counts.begin(), TPX3_SENSOR_SIZE, r);

    for(auto &cluster : mCentroids) {
        auto px_x = static_cast<unsigned>(cluster.x / PIXEL_SIZE);
        auto px_y = static_cast<unsigned>(cluster.y / PIXEL_SIZE);
        ++pixel_counts[px_x][px_y];
    }

    return pixel_counts;

}

std::pair<QVector<double>, QVector<double>> Tpx3Image::startStopHistogram(double hist_bin_size, unsigned num_bins) const {

    unsigned num_clusters = numClusters();

    if(num_clusters < 2)
        throw std::runtime_error("Need at least two clusters to create a start-stop histogram.");

    std::vector<unsigned> bin_values(num_bins, 0);

    for(unsigned ix = 0; ix < num_clusters - 1; ++ix) {
        double startstop = mCentroids[ix + 1].toa - mCentroids[ix].toa;
        // rounds down; MIN_TICK/2 moves values from edges of bins to center, so there is less numerical artifacts
        unsigned bin_ix = static_cast<unsigned>((startstop + MIN_TICK/2) / hist_bin_size);
        if(bin_ix < num_bins)
            ++bin_values[bin_ix];
    }

    bin_values[0] *= 2; // accounts for the fact that we only traverse the array in one direction, which undercounts zero delays

    QVector<double> x(num_bins), y(num_bins);
    for (int i=0; i<num_bins; ++i) {
        x[i] = i*hist_bin_size;
        y[i] = bin_values[i];
    }

    return std::make_pair<QVector<double>, QVector<double>>(std::move(x), std::move(y));

}

std::tuple<QVector<double>, QVector<double>, QVector<double>> Tpx3Image::dToADistribution(unsigned int hist_bin_size) const {

    unsigned num_clusters = numClusters();
    unsigned num_packets = numRawPackets();
    constexpr unsigned num_tot = 1024;

    auto hist_size = static_cast<unsigned>(std::ceil(static_cast<float>(num_tot) / static_cast<float>(hist_bin_size)));

    std::vector<unsigned> tot_count(num_tot, 0);
    std::vector<double> dtoa_means(num_tot, 0); // mean dToA

    // now histogram
    std::vector<double> x(hist_size, 0);
    std::vector<double> y(hist_size, 0);

    auto &toa_arr = data().toa;
    auto &tot_arr = data().tot;

    // corrected two-pass algorithm to calculate mean and st.dev

    // Mean first:
    for(unsigned p = 0; p < num_packets; ++p) {
        auto tot = tot_arr[p];
        if(tot >= num_tot)
            continue;

        if(mClusters.cluster_ids[p] == 0) // not in a cluster
            continue;

        auto cluster_id = mClusters.cluster_ids[p] - 1;
        auto dToA = static_cast<double>(toa_arr[p])*MIN_TICK - mCentroids[cluster_id].toa;

        tot_count[tot] += 1;
        dtoa_means[tot] += dToA;
    }

    for(unsigned id = 0; id < num_tot; ++id) {
        if(tot_count[id])
            dtoa_means[id] /= tot_count[id];
    }

    // Now st. dev:
    std::vector<double> dtoa_sq_err(num_tot, 0); // mean dToA
    std::vector<double> dtoa_err(num_tot, 0); // mean dToA

    for(unsigned p = 0; p < num_packets; ++p) {
        auto tot = tot_arr[p];
        if(tot >= num_tot)
            continue;

        if(mClusters.cluster_ids[p] == 0) // not in a cluster
            continue;

        auto cluster_id = mClusters.cluster_ids[p] - 1;
        auto dToA = static_cast<double>(toa_arr[p])*MIN_TICK - mCentroids[cluster_id].toa;
        auto err = dToA - dtoa_means[tot];

        dtoa_err[tot] += err;
        dtoa_sq_err[tot] += err*err;
    }

    // prep data for Qt

    QVector<double> qt_x, qt_y, qt_yerr;
    qt_x.reserve(num_tot);
    qt_y.reserve(num_tot);
    qt_yerr.reserve(num_tot);

    for(auto ix = 0; ix < x.size(); ++ix) {
        qt_x.push_back(ix * TOT_UNIT_SIZE);
        qt_y.push_back(dtoa_means[ix]);

        auto N = tot_count[ix];
        if(N) {
            auto stdev = std::sqrt((dtoa_sq_err[ix] - (dtoa_err[ix] * dtoa_err[ix]) / N) / (N - 1));
            qt_yerr.push_back(stdev); // std. error in the mean
        } else {
            qt_yerr.push_back(0);
        }
    }

    return std::make_tuple<QVector<double>, QVector<double>, QVector<double>>(std::move(qt_x), std::move(qt_y), std::move(qt_yerr));

}

ImageXY<unsigned> Tpx3Image::spatialCorrelations() const {

    std::vector<unsigned> r;
    r.insert(r.begin(), Tpx3Image::SPATIAL_CORR_SIZE, 0);

    ImageXY<unsigned> pixel_counts;
    pixel_counts.insert(pixel_counts.begin(), Tpx3Image::SPATIAL_CORR_SIZE, r);

    for(auto &biphoton : mBiphotonClicks) {
        auto px_x = static_cast<unsigned>(biphoton.wl_1 / PIXEL_SIZE * Tpx3Image::SPATIAL_CORR_SIZE / TPX3_SENSOR_SIZE);
        auto px_y = static_cast<unsigned>(biphoton.wl_2 / PIXEL_SIZE * Tpx3Image::SPATIAL_CORR_SIZE / TPX3_SENSOR_SIZE);

        if(biphoton.channel_1 != biphoton.channel_2)
            ++pixel_counts[px_x][px_y];
    }

    return pixel_counts;

}

void Tpx3Image::initializeSpectrum() {

    // Look for peaks along horizontal and vertical direction, and pick direction accordingly
    std::vector<unsigned> h_slice, v_slice;
    h_slice.reserve(Tpx3Image::HEIGHT);
    v_slice.reserve(Tpx3Image::WIDTH);

    auto raw_image = rawPacketImage();

    for (unsigned r = 0; r < Tpx3Image::HEIGHT; ++r) {
        unsigned slice_tot = 0;
        for (unsigned c = 0; c < Tpx3Image::WIDTH; ++c) {
            slice_tot += raw_image[r][c];
        }
        v_slice.push_back(slice_tot);
    }
    for (unsigned c = 0; c < Tpx3Image::WIDTH; ++c) {
        unsigned slice_tot = 0;
        for (unsigned r = 0; r < Tpx3Image::HEIGHT; ++r) {
            slice_tot += raw_image[r][c];
        }
        h_slice.push_back(slice_tot);
    }

    auto h_slice_max = *std::max_element(h_slice.cbegin(), h_slice.cend());
    auto v_slice_max = *std::max_element(v_slice.cbegin(), v_slice.cend());

    bool h_lines = (h_slice_max >= v_slice_max);

    // fit the lines
    LinePair lines = LinePair::find(raw_image, h_lines);

    mBiphotonClicks.clear();
    mBiphotonClicks.reserve(mCoincidencePairs.size());

    for(auto &coinc : mCoincidencePairs) {
        auto centroid1 = mCentroids[coinc.id_1];
        auto centroid2 = mCentroids[coinc.id_2];

        int channel1 = lines.closestLine(centroid1.x, centroid1.y);
        int channel2 = lines.closestLine(centroid2.x, centroid2.y);

        // TODO: wavelength correction
        double pix1, pix2;
        if(h_lines) {
            pix1 = centroid1.x;
            pix2 = centroid2.x;
        } else {
            pix1 = centroid1.y;
            pix2 = centroid2.y;
        }

        mBiphotonClicks.push_back({
            pix1,
            pix2,
            channel1,
            channel2
        });
    }

}

void Tpx3Image::saveTo(const std::string &coinc_path, const std::string &singles_path) const {

    std::ofstream coinc_file(coinc_path);
    coinc_file << "Channel 1, Channel 2, Wavelength 1, Wavelength 2\n";
    for(auto &biphoton : mBiphotonClicks)
        coinc_file << biphoton.channel_1 << ", " << biphoton.channel_2 << ", " << biphoton.wl_1 << ", " << biphoton.wl_2 << "\n";
    coinc_file << std::flush;

    std::ofstream singles_file(singles_path);
    singles_file << "X [m], Y [m]\n";
    for(auto &cluster : mCentroids)
        singles_file << cluster.x << ", " << cluster.y << "\n";
    singles_file << std::flush;

}
