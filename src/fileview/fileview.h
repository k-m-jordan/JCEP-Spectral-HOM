#ifndef SPECTRAL_HOM_FILEVIEW_H
#define SPECTRAL_HOM_FILEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

class QCustomPlot;

namespace spec_hom {

    class Tpx3Image;

    class RawImageView : public QWidget {
    public:
        RawImageView(QWidget *parent, Tpx3Image *src);

    private:
        QVBoxLayout *mLayout;
        QCustomPlot *mPlot;
    };

    class ToTDistributionView : public QWidget {
    public:
        ToTDistributionView(QWidget *parent, Tpx3Image *src);

    private:
        QVBoxLayout *mLayout;
        QCustomPlot *mPlot;
    };

    class ClusteredImageView : public QWidget {
    public:
        ClusteredImageView(QWidget *parent, Tpx3Image *src);

    private:
        QVBoxLayout *mLayout;
        QCustomPlot *mPlot;
    };

    class StartStopHistogramView : public QWidget {
    public:
        StartStopHistogramView(QWidget *parent, Tpx3Image *src);

    private:
        QVBoxLayout *mLayout;
        QCustomPlot *mPlot;
    };

    class DToADistributionView : public QWidget {
        Q_OBJECT
    public:
        DToADistributionView(QWidget *parent, Tpx3Image *src);

    private:
        void exportCalibrationClicked();

        QVector<double> mXData, mYData;

        QVBoxLayout *mLayout;
        QCustomPlot *mPlot;
        QPushButton *mExportCalibButton;
    };

    class FileViewer : public QWidget {
        Q_OBJECT
    public:
        FileViewer(QWidget *parent, Tpx3Image *image);

        [[nodiscard]] const std::string& fullFilename() const;

    private slots:
        void viewTypeChanged(int newIndex);

    private:
        Tpx3Image *mImage;

        QVBoxLayout *mLayout;
        QGroupBox *mCenterWidget;
        QVBoxLayout *mCenterLayout;
        QComboBox *mViewSelection;
        QWidget *mViewContainer;
    };

}

#endif //SPECTRAL_HOM_FILEVIEW_H
