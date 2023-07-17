#include "tpx3.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>

#include <QImage>

using namespace spec_hom;

Tpx3Image::Tpx3Image(std::string fname, PixelData &&raw_data, ClusterData &&clusters, std::vector<ClusterEvent> &&centroids) :
        mFileName(std::move(fname)),
        mRawData(std::move(raw_data)),
        mClusters(std::move(clusters)),
        mCentroids(std::move(centroids)) {

    // Do nothing

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

Tpx3ImageXY<unsigned> Tpx3Image::rawPacketImage() const {

    Tpx3ImageXY<unsigned> pixel_counts;
    for(auto &row : pixel_counts)
        for(auto &x : row)
            x = 0;

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

Tpx3ImageXY<unsigned> Tpx3Image::clusterImage() const {

    Tpx3ImageXY<unsigned> pixel_counts;
    for(auto &row : pixel_counts)
        for(auto &x : row)
            x = 0;

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

std::tuple<QVector<double>, QVector<double>, QVector<double>> Tpx3Image::dToTDistribution(unsigned int hist_bin_size) const {

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

    for(auto ix = 0; ix < qt_x.size(); ++ix) {
        std::cout << qt_x[ix] << "," << (qt_y[ix] / MIN_TICK) << std::endl;
    }

    return std::make_tuple<QVector<double>, QVector<double>, QVector<double>>(std::move(qt_x), std::move(qt_y), std::move(qt_yerr));

}