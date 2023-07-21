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

    class FileViewPanel : public QWidget {
        Q_OBJECT
    public:
        FileViewPanel(QWidget *parent, const Tpx3Image *src);

        void saveImageBtnClick();
        virtual void saveDataBtnClick() = 0;

    protected:
        [[nodiscard]] const Tpx3Image* source() const { return mSrcImage; }
        [[nodiscard]] QCustomPlot* plot() { return mPlot; }
        [[nodiscard]] QLayout* toolbarLayout() const { return mToolbarLayout; }

    private:
        const Tpx3Image *mSrcImage;

        QVBoxLayout *mLayout;
        QCustomPlot *mPlot;
        QWidget *mToolbar;
        QHBoxLayout *mToolbarLayout;

        // Toolbar buttons
        QPushButton *mSaveImageBtn;
        QPushButton *mExportDataBtn;
    };

    class LinePlotView : public FileViewPanel {
    public:
        LinePlotView(QWidget *parent, const Tpx3Image *src);

        void saveDataBtnClick() override;

    protected:
        [[nodiscard]] QVector<double>& xData() { return mXData; }
        [[nodiscard]] QVector<double>& yData() { return mYData; }
        [[nodiscard]] QVector<double>& yErr() { return mYErr; }

        void title(const QString &label){ mTitle = label; }
        void xLabel(const QString &label){ mXLabel = label; }
        void yLabel(const QString &label){ mYLabel = label; }

        void updatePlot();

    private:
        QVector<double> mXData, mYData, mYErr;
        QString mTitle;
        QString mXLabel, mYLabel;
    };

    class Hist2DView : public FileViewPanel {
    public:
        Hist2DView(QWidget *parent, const Tpx3Image *src);

        void saveDataBtnClick() override;

    protected:
        [[nodiscard]] std::vector<std::vector<double>>& data() { return mData; }

        void title(const QString &label){ mTitle = label; }
        void xLabel(const QString &label){ mXLabel = label; }
        void yLabel(const QString &label){ mYLabel = label; }
        void colorbarLabel(const QString &label){ mColorbarLabel = label; }

        void updatePlot();

    private:
        std::vector<std::vector<double>> mData;
        QString mTitle;
        QString mXLabel, mYLabel;
        QString mColorbarLabel;
    };

    class RawImageView : public Hist2DView {
    public:
        RawImageView(QWidget *parent, Tpx3Image *src);
    };

    class ToTDistributionView : public LinePlotView {
    public:
        ToTDistributionView(QWidget *parent, Tpx3Image *src);
    };

    class ClusteredImageView : public Hist2DView {
    public:
        ClusteredImageView(QWidget *parent, Tpx3Image *src);
    };

    class StartStopHistogramView : public LinePlotView {
    public:
        StartStopHistogramView(QWidget *parent, Tpx3Image *src);
    };

    class DToADistributionView : public LinePlotView {
        Q_OBJECT
    public:
        DToADistributionView(QWidget *parent, Tpx3Image *src);

    private:
        void exportCalibrationClicked();

        QPushButton *mExportCalibButton;
    };

    class SpatialCorrelationView : public Hist2DView {
    public:
        SpatialCorrelationView(QWidget *parent, Tpx3Image *src);
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
