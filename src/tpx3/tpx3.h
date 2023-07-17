#ifndef SPECTRAL_HOM_TPX3_H
#define SPECTRAL_HOM_TPX3_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <tuple>
#include <array>

#include <QRunnable> // used to allow communications between the background thread and the UI
#include <QObject>

#include <dlib/optimization.h>

#include "ui/threadutils.h"

namespace spec_hom {

    constexpr double PIXEL_SIZE = 55e-6;
    constexpr double MIN_TICK = 1.5625e-9;
    constexpr int TPX3_SENSOR_SIZE = 256;
    constexpr double TOT_UNIT_SIZE = 25e-9; // data in units of 25 ns

    using ToTCalibration = std::array<double, 1024>;

    struct SpatialMask {
        bool vertical; // horizontal if false
        int min1, max1, min2, max2; // min and max indices of the two lines
    };

    struct Tpx3ImportSettings {
        int maxNumThreads;
        SpatialMask spatialMask;

        ToTCalibration totCorrection;

        float clusterSizeXY;
        float clusterSizeT;
        int minClusterSize;
    };

    struct PixelAddr {
        uint8_t x;
        uint8_t y;
    };

    struct PixelData {
        std::vector<PixelAddr> addr;
        std::vector<int64_t> toa;
        std::vector<uint16_t> tot;

        [[nodiscard]] unsigned long memsize() const;
        [[nodiscard]] unsigned long numPackets() const;
        [[nodiscard]] bool isEmpty() const;
    };

    struct ClusterData {
        int num_clusters;
        std::vector<int> cluster_ids;
    };

    struct ClusterEvent {
        double x, y; // physical position [m]
        double toa; // physical time of arrival [s]
    };

    template<typename T>
    using Tpx3ImageXY = std::array<std::array<T, TPX3_SENSOR_SIZE>, TPX3_SENSOR_SIZE>;

    class LinePair {
    public:
        LinePair(bool vertical, double line_1_pos, double line_2_pos, double line_1_sigma, double line_2_sigma);

        // If these pointers are supplied, this function will return the fit data used
        static LinePair find(Tpx3ImageXY<unsigned> &image, bool h_lines, QVector<double> *x = nullptr, QVector<double> *y = nullptr, QVector<double> *fit_y = nullptr);

        void getRectBounds(double &min1, double &max1, double &min2, double &max2, double num_sigma);

    private:
        bool mIsVertical; // whether the lines are vertical (true) or horizontal (false)
        double mLine1Pos, mLine2Pos; // [um]
        double mLine1Sigma, mLine2Sigma; // [um]
    };

    class Tpx3Image {
    public:
        static constexpr unsigned WIDTH = TPX3_SENSOR_SIZE, HEIGHT = TPX3_SENSOR_SIZE;

        Tpx3Image(std::string fname, PixelData &&raw_data, ClusterData &&clusters, std::vector<ClusterEvent> &&centroids);
        Tpx3Image(const Tpx3Image &rhs) = delete; // this object is large; better to avoid unnecessary copies
        ~Tpx3Image() = default;

        [[nodiscard]] std::string filename() const;
        [[nodiscard]] const std::string& fullFilename() const;
        PixelData& data();
        [[nodiscard]] const PixelData& data() const;
        [[nodiscard]] unsigned long numRawPackets() const;
        [[nodiscard]] unsigned long numClusters() const;
        [[nodiscard]] bool empty() const;

        [[nodiscard]] Tpx3ImageXY<unsigned> rawPacketImage() const;
        [[nodiscard]] std::pair<QVector<double>, QVector<double>> toTDistribution(unsigned hist_bin_size = 1) const;
        [[nodiscard]] Tpx3ImageXY<unsigned> clusterImage() const;
        [[nodiscard]] std::pair<QVector<double>, QVector<double>> startStopHistogram(double hist_bin_size = MIN_TICK, unsigned num_bins = 128) const; // hist_size in seconds
        [[nodiscard]] std::tuple<QVector<double>, QVector<double>, QVector<double>> dToTDistribution(unsigned hist_bin_size = 1) const;

    private:
        std::string mFileName;
        PixelData mRawData;
        ClusterData mClusters;
        std::vector<ClusterEvent> mCentroids;
    };

    // Loads a given file into a vector of PixelData's
    class LoadRawFileThread : public BgThread {
    Q_OBJECT

    public:
        LoadRawFileThread(const std::string &fname, Tpx3ImportSettings settings, bool raw_packets_only = false);

        void execute() override;

    signals:
        void yieldPixelData(spec_hom::Tpx3Image *data);

    private:
        void finish(PixelData &&data, ClusterData &&clusters, std::vector<ClusterEvent> &&centroids);

        PixelData parseRawData();
        ClusterData cluster(const PixelData &data);
        std::vector<ClusterEvent> centroid(const PixelData &data, const ClusterData &clusters);

        std::string mFileName;
        Tpx3ImportSettings mImportSettings;
        bool mRawPacketsOnly;
    };

}

#endif //SPECTRAL_HOM_TPX3_H
