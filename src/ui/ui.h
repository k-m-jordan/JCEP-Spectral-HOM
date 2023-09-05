#ifndef SPECTRAL_HOM_UI_H
#define SPECTRAL_HOM_UI_H

#include <memory>
#include <vector>
#include <map>
#include <chrono>

#include <QWidget>
#include <QLayout>
#include <QPushButton>
#include <QMenuBar>
#include <QMainWindow>
#include <QLabel>
#include <QSplitter>
#include <QFrame>
#include <QProgressDialog>
#include <QProgressBar>
#include <QTableWidget>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>

#include <dlib/optimization.h>

class QCustomPlot;
class QCPItemRect;

#include "log.h"
#include "tpx3/tpx3.h"

namespace spec_hom {

    class Tpx3Image;
    class PixelData;
    class MainWindow;
    class BgThread;
    class LoadRawFileThread;
    class FileViewer;

    // actions for the global app that are launched by sub-panels of the UI
    struct AppActions {
        QAction *close;
        QAction *openFilesDialog;
        QAction *startImportFiles;
        QAction *stopImportFiles;
        QAction *clearFileList;
        QAction *exportAllData;
        QAction *lockUiForImporting;
        QAction *doneImporting;
        QAction *deleteFile; // to use this action, set its data to a QString or QStringList with the filenames to delete
        QAction *openFileTab; // to use this action, set its data to a QString with the filename to open
        QAction *lockUiForMasking;
        QAction *unlockUi;

        explicit AppActions(QWidget *parent);
    };

    class FileImportProgressBar : public QProgressBar {
        Q_OBJECT
    public:
        explicit FileImportProgressBar(QWidget *parent);

        [[nodiscard]] std::string label() const;
        [[nodiscard]] QColor labelColor() const;

        [[nodiscard]] QString text() const override; // override allows for text to be displayed on top of indefinite progress bar

        [[nodiscard]] bool queued() const { return mIsQueued; }
        [[nodiscard]] bool loaded() const { return mIsLoaded; }

        void setQueued();
        void setLoaded();

        [[nodiscard]] bool isLoading() const { return mIsLoading; }
        void setIsLoading();

        FileImportProgressBar* clone(QWidget *parent) const;

        void connectThread(LoadRawFileThread *thread);
        void connectThread(const FileImportProgressBar *other);

    public slots:
        void setIndefinite(bool value);
        void setLabel(std::string txt);
        void setLabelColor(QColor color);

        void disconnectThread();

    private:
        LoadRawFileThread *mConnectedThread; // needed so that we can clone properly
        bool mIsQueued; // whether file is queued
        bool mIsLoading; // whether the file is currently being loaded
        bool mIsLoaded; // whether the file is done loading
    };

    class SetImageMaskDialog : public QDialog {
        Q_OBJECT
    public:
        SetImageMaskDialog(std::unique_ptr<Tpx3Image> &&reference, QWidget *parent);

    signals:
        void yieldImageMask(spec_hom::SpatialMask, std::string filename);

    private:
        void updateSlicePlot();
        void acceptClicked();

        std::unique_ptr<Tpx3Image> mRefImage;
        ImageXY<unsigned> mRawImage;
        LinePair mLastFit;

        QHBoxLayout *mLayout;

        QWidget *mLeftWidget;
        QVBoxLayout *mLeftLayout;
        QCustomPlot *mImagePlot;

        QWidget *mRightWidget;
        QVBoxLayout *mRightLayout;

        QCustomPlot *mFitPlot;

        QWidget *mLineDirWidget;
        QHBoxLayout *mLineDirLayout;
        QRadioButton *mHorButton;
        QRadioButton *mVertButton;

        QWidget *mSigmaWidget;
        QHBoxLayout *mSigmaLayout;
        QLabel *mSigmaLabel;
        QSlider *mSigmaSlider;

        QPushButton *mAcceptButton;
    };

    class FileInputSettingsPanel : public QWidget {
        Q_OBJECT
    public:
        FileInputSettingsPanel(QWidget *parent, AppActions &actions);

        Tpx3ImportSettings getSettings();
        bool shouldExportSingles() const;

    private slots:
        void receiveImageMask(spec_hom::SpatialMask mask, std::string filename);

    private:
        void setImageMaskClick();
        void clearImageMaskClick();
        void receiveImageMaskReference(Tpx3Image *image);
        void setToACalibClick();
        void clearToACalibClick();
        void loadCalibrationFileClick();

        AppActions &mActions;
        std::unique_ptr<SpatialMask> mCurrImageMask;
        ToTCalibration mCurrCalibration;

        QVBoxLayout *mLayout;

        QGroupBox *mGeneralSettingsWidget;                  // General import settings
        QVBoxLayout *mGeneralSettingsLayout;
        QWidget *mNumThreadsWidget;                         // Number of threads to use for imports
        QHBoxLayout *mNumThreadsLayout;
        QLabel *mNumThreadsLabel;
        QSpinBox *mNumThreadsSpinbox;
        QWidget *mSpatialMaskWidget;
        QHBoxLayout *mSpatialMaskLayout;
        QLabel *mSpatialMaskLabel;
        QLabel *mSpatialMaskCurrLabel;
        QPushButton *mSpatialMaskSetBtn;
        QPushButton *mSpatialMaskClearBtn;

        QGroupBox *mToTCorrectionSettingsWidget;            // Settings for ToT correction
        QVBoxLayout *mToTCorrectionSettingsLayout;
        QWidget *mToTCorrectionSourceWidget;                // Calibration data to use for ToT correction
        QHBoxLayout *mToTCorrectionSourceLayout;
        QLabel *mToTCorrLabel;
        QLabel *mToTCorrCurrLabel;
        QPushButton *mToTCorrSetBtn;
        QPushButton *mToTCorrClearBtn;

        QGroupBox *mClusteringSettingsWidget;               // Settings for clustering
        QVBoxLayout *mClusteringSettingsLayout;
        QWidget *mClusterWindowWidget;                      // Size of clustering window
        QHBoxLayout *mClusterWindowLayout;
        QLabel *mClusterWindowXYLabel;
        QLineEdit *mClusterWindowXYEdit;
        QLabel *mClusterWindowTLabel;
        QLineEdit *mClusterWindowTEdit;
        QWidget *mMinClusterSizeWidget;                     // Minimum number of packets in a cluster
        QHBoxLayout *mMinClusterSizeLayout;
        QLabel *mMinClusterSizeLabel;
        QSpinBox *mMinClusterSizeEdit;

        QGroupBox *mCoincidenceSettingsWidget;
        QVBoxLayout *mCoincidenceSettingsLayout;
        QWidget *mCoincidenceWindowWidget;
        QHBoxLayout *mCoincidenceWindowLayout;
        QLabel *mCoincidenceWindowLabel;
        QLineEdit *mCoincidenceWindowEdit;

        QGroupBox *mCalibrationSettingsWidget;
        QVBoxLayout *mCalibrationSettingsLayout;
        QWidget *mCalibration1Widget;
        QHBoxLayout *mCalibration1Layout;
        QLabel *mCalibrationSlope1Label;
        QLineEdit *mCalibrationSlope1Edit;
        QLabel *mCalibrationIntercept1Label;
        QLineEdit *mCalibrationIntercept1Edit;
        QWidget *mCalibration2Widget;
        QHBoxLayout *mCalibration2Layout;
        QLabel *mCalibrationSlope2Label;
        QLineEdit *mCalibrationSlope2Edit;
        QLabel *mCalibrationIntercept2Label;
        QLineEdit *mCalibrationIntercept2Edit;
        QPushButton *mLoadCalibrationBtn;

        QGroupBox *mExportSettingsWidget;
        QVBoxLayout *mExportSettingsLayout;
        QCheckBox *mExportSinglesCheck;

        QLabel *mBottomText;

    };

    class FileInputPanel : public QWidget {
        Q_OBJECT
    public:
        FileInputPanel(QWidget *parent, LogPanel &logger, AppActions &actions);

        void openFileDialog();
        void removeTableRow(int row);
        void clearAllRows();
        void setCancelBtnOnly(bool value);

        [[nodiscard]] const std::vector<std::string>& fileList() const { return mFileList; }
        [[nodiscard]] std::vector<std::string> queuedFileList() const;

        int getFileRow(const std::string &file);
        void connectThread(const std::string &file, LoadRawFileThread *thread);
        void setFileLoaded(const std::string &file);
        void setFileQueued(const std::string &file);

    public slots:
        void startStopBtnClick();
        void tableDoubleClick(int row, int col);

    private:
        void addQueuedFiles(std::vector<std::string> paths);
        void updateFileTable();
        void updateLoadStatus();

        AppActions &mActions;
        LogPanel &mLogger;
        QVBoxLayout *mLayout;

        QWidget *mToolbar;
        QHBoxLayout *mToolbarLayout;
        QPushButton *mOpenFileBtn;
        QPushButton *mStartStopLoadBtn;
        QPushButton *mClearFilesBtn;
        QPushButton *mExportFilesBtn;

        QTableWidget *mFileTable;

        QLabel *mBottomText;

        bool mCancelBtnOnly;

        std::vector<std::string> mFileList;
    };

    class MainWindow : public QMainWindow {
        Q_OBJECT
    public:
        MainWindow();
        ~MainWindow() override;

    public slots:
        void freezeUiForImporting();
        void freezeUiForMasking();
        void unfreezeUi();

        void exportAllData();
        void deleteFiles(); // note: this requires that the filename be stored in the corresponding QAction's data()
        void deleteAllFiles();
        void startImportFiles();
        void stopImportFiles();
        void doneImportFiles();
        void receiveImageData(spec_hom::Tpx3Image *data);

        void openNewFileTab(); // note: this requires that the filename be stored in the corresponding QAction's data()
        void closeFileTab(int index);

        bool closeWindow();
        void closeEvent(QCloseEvent *event) override;

    private:
        AppActions mActions;

        QSplitter *mCentralSplitter; // Splitter containing the log panel (left) and the viewing panel (right)

        LogPanel *mLogPanel; // Logging panel containing info & error messages

        QTabWidget *mTabContainer; // Tab container for main tools
        FileInputSettingsPanel *mFileSettingsPanel; // Settings used to import tpx3 files
        FileInputPanel *mFilePanel; // Panel to open files
        std::vector<LoadRawFileThread*> mActiveImportThreads;
        std::map<std::string, std::unique_ptr<Tpx3Image>> mOpenImages;
        std::map<std::string, FileViewer*> mOpenFileViewTabs;

        decltype(std::chrono::high_resolution_clock::now()) mProcessStartTime;
    };

}

#endif //SPECTRAL_HOM_UI_H
