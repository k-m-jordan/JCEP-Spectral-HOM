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

        double coincidenceWindow;
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

    struct ClusterCentroid {
        double x, y; // physical position [m]
        double toa; // physical time of arrival [s]
    };

    struct CoincidencePair {
        unsigned id_1, id_2; // cluster ids for the two coincident events
    };

    struct CoincidenceNFold {
        std::vector<unsigned> ids; // cluster ids for the n coincident events
    };

    struct SpectrumPair {
        double wl_1, wl_2; // wavelengths [m] of two photons
        int channel_1, channel_2; // which beam (top=1, bottom=2) the photon was in
    };

    template<typename T>
    using ImageXY = std::vector<std::vector<T>>;

    class LinePair {
    public:
        LinePair(bool vertical, double line_1_pos, double line_2_pos, double line_1_sigma, double line_2_sigma);

        // If these pointers are supplied, this function will return the fit data used
        static LinePair find(ImageXY<unsigned> &image, bool h_lines, QVector<double> *x = nullptr, QVector<double> *y = nullptr, QVector<double> *fit_y = nullptr);

        void getRectBounds(double &min1, double &max1, double &min2, double &max2, double num_sigma);
        int closestLine(double x, double y); // returns 1 if left line is nearest, 2 if right line is nearest (does not use sigma)

    private:
        bool mIsVertical; // whether the lines are vertical (true) or horizontal (false)
        double mLine1Pos, mLine2Pos; // [um]
        double mLine1Sigma, mLine2Sigma; // [um]
    };

    class Tpx3Image {
    public:
        static constexpr unsigned WIDTH = TPX3_SENSOR_SIZE, HEIGHT = TPX3_SENSOR_SIZE;

        Tpx3Image(std::string fname, PixelData &&raw_data, ClusterData &&clusters, std::vector<ClusterCentroid> &&centroids,
                  std::vector<CoincidencePair> &&coinc_pairs, std::vector<CoincidenceNFold> &&coinc_nfolds);
        Tpx3Image(const Tpx3Image &rhs) = delete; // this object is large; better to avoid unnecessary copies
        ~Tpx3Image() = default;

        [[nodiscard]] std::string filename() const;
        [[nodiscard]] const std::string& fullFilename() const;
        PixelData& data();
        [[nodiscard]] const PixelData& data() const;
        [[nodiscard]] unsigned long numRawPackets() const;
        [[nodiscard]] unsigned long numClusters() const;
        [[nodiscard]] bool empty() const;

        [[nodiscard]] ImageXY<unsigned> rawPacketImage() const;
        [[nodiscard]] std::pair<QVector<double>, QVector<double>> toTDistribution(unsigned hist_bin_size = 1) const;
        [[nodiscard]] ImageXY<unsigned> clusterImage() const;
        [[nodiscard]] std::pair<QVector<double>, QVector<double>> startStopHistogram(double hist_bin_size = MIN_TICK, unsigned num_bins = 128) const; // hist_size in seconds
        [[nodiscard]] std::tuple<QVector<double>, QVector<double>, QVector<double>> dToADistribution(unsigned hist_bin_size = 1) const;

        static constexpr int SPATIAL_CORR_SIZE = TPX3_SENSOR_SIZE;
        [[nodiscard]] ImageXY<unsigned> spatialCorrelations() const;

        void saveTo(const std::string &coinc_path, const std::string &singles_path) const;

    private:
        void initializeSpectrum();

        std::string mFileName;
        PixelData mRawData;
        ClusterData mClusters;
        std::vector<ClusterCentroid> mCentroids;
        std::vector<CoincidencePair> mCoincidencePairs;
        std::vector<CoincidenceNFold> mCoincidenceNFold;
        std::vector<SpectrumPair> mBiphotonClicks;
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
        void finish(PixelData &&data, ClusterData &&clusters, std::vector<ClusterCentroid> &&centroids,
                    std::vector<CoincidencePair> &&coinc_pairs, std::vector<CoincidenceNFold> &&coinc_nfolds);
        void finish(); // calls previous function, but with all arguments initialized from empty list

        PixelData parseRawData();
        ClusterData cluster(const PixelData &data);
        std::vector<ClusterCentroid> centroid(const PixelData &data, const ClusterData &clusters);
        std::pair<std::vector<CoincidencePair>, std::vector<CoincidenceNFold>> findCoincidences(const ClusterData &clusters, const std::vector<ClusterCentroid> &centroids);

        std::string mFileName;
        Tpx3ImportSettings mImportSettings;
        bool mRawPacketsOnly;
    };

}

#endif //SPECTRAL_HOM_TPX3_H
