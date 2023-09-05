#include "ui.h"

#include <iostream>
#include <fstream>

#include <QThread>
#include <QThreadPool>
#include <QFileDialog>
#include <QProgressDialog>

using namespace spec_hom;

// ToT correction for the ToA
int64_t tot_correction(uint16_t tot) {

    // Hard-coded fit of tot vs. correction
    // Taken from Guillaume's data
    const static float a = 5.1761, b = -0.001, c = -83.2854, d = -0.0039;

    auto x = static_cast<float>(tot);
    auto correction = a*std::exp(b*x) + c*std::exp(d*x);
    return std::llround(correction);

}

FileInputSettingsPanel::FileInputSettingsPanel(QWidget *parent, AppActions &actions) :
        QWidget(parent),
        mActions(actions),
        mCurrImageMask(nullptr),
        mCurrCalibration(),

        mLayout(new QVBoxLayout(this)),

        mGeneralSettingsWidget(new QGroupBox(this)),
        mGeneralSettingsLayout(new QVBoxLayout(mGeneralSettingsWidget)),
        mNumThreadsWidget(new QWidget(mGeneralSettingsWidget)),
        mNumThreadsLayout(new QHBoxLayout(mNumThreadsWidget)),
        mNumThreadsLabel(new QLabel(mNumThreadsWidget)),
        mNumThreadsSpinbox(new QSpinBox(mNumThreadsWidget)),
        mSpatialMaskWidget(new QWidget(mGeneralSettingsWidget)),
        mSpatialMaskLayout(new QHBoxLayout(mSpatialMaskWidget)),
        mSpatialMaskLabel(new QLabel(mSpatialMaskWidget)),
        mSpatialMaskCurrLabel(new QLabel(mSpatialMaskWidget)),
        mSpatialMaskSetBtn(new QPushButton(mSpatialMaskWidget)),
        mSpatialMaskClearBtn(new QPushButton(mSpatialMaskWidget)),

        mToTCorrectionSettingsWidget(new QGroupBox(this)),
        mToTCorrectionSettingsLayout(new QVBoxLayout(mToTCorrectionSettingsWidget)),
        mToTCorrectionSourceWidget(new QWidget(mToTCorrectionSettingsWidget)),
        mToTCorrectionSourceLayout(new QHBoxLayout(mToTCorrectionSourceWidget)),
        mToTCorrLabel(new QLabel(mSpatialMaskWidget)),
        mToTCorrCurrLabel(new QLabel(mSpatialMaskWidget)),
        mToTCorrSetBtn(new QPushButton(mSpatialMaskWidget)),
        mToTCorrClearBtn(new QPushButton(mSpatialMaskWidget)),

        mClusteringSettingsWidget(new QGroupBox(this)),
        mClusteringSettingsLayout(new QVBoxLayout(mClusteringSettingsWidget)),
        mClusterWindowWidget(new QWidget(mClusteringSettingsWidget)),
        mClusterWindowLayout(new QHBoxLayout(mClusterWindowWidget)),
        mClusterWindowXYLabel(new QLabel(mClusterWindowWidget)),
        mClusterWindowXYEdit(new QLineEdit(mClusterWindowWidget)),
        mClusterWindowTLabel(new QLabel(mClusterWindowWidget)),
        mClusterWindowTEdit(new QLineEdit(mClusterWindowWidget)),
        mMinClusterSizeWidget(new QWidget(mClusteringSettingsWidget)),
        mMinClusterSizeLayout(new QHBoxLayout(mMinClusterSizeWidget)),
        mMinClusterSizeLabel(new QLabel(mMinClusterSizeWidget)),
        mMinClusterSizeEdit(new QSpinBox(mMinClusterSizeWidget)),

        mCoincidenceSettingsWidget(new QGroupBox(this)),
        mCoincidenceSettingsLayout(new QVBoxLayout(mCoincidenceSettingsWidget)),
        mCoincidenceWindowWidget(new QWidget(mCoincidenceSettingsWidget)),
        mCoincidenceWindowLayout(new QHBoxLayout(mCoincidenceWindowWidget)),
        mCoincidenceWindowLabel(new QLabel(mCoincidenceWindowWidget)),
        mCoincidenceWindowEdit(new QLineEdit(mCoincidenceWindowWidget)),

        mCalibrationSettingsWidget(new QGroupBox(this)),
        mCalibrationSettingsLayout(new QVBoxLayout(mCalibrationSettingsWidget)),
        mCalibration1Widget(new QWidget(mCalibrationSettingsWidget)),
        mCalibration1Layout(new QHBoxLayout(mCalibration1Widget)),
        mCalibrationSlope1Label(new QLabel(mCalibration1Widget)),
        mCalibrationSlope1Edit(new QLineEdit(mCalibration1Widget)),
        mCalibrationIntercept1Label(new QLabel(mCalibration1Widget)),
        mCalibrationIntercept1Edit(new QLineEdit(mCalibration1Widget)),
        mCalibration2Widget(new QWidget(mCalibrationSettingsWidget)),
        mCalibration2Layout(new QHBoxLayout(mCalibration2Widget)),
        mCalibrationSlope2Label(new QLabel(mCalibration2Widget)),
        mCalibrationSlope2Edit(new QLineEdit(mCalibration2Widget)),
        mCalibrationIntercept2Label(new QLabel(mCalibration2Widget)),
        mCalibrationIntercept2Edit(new QLineEdit(mCalibration2Widget)),
        mLoadCalibrationBtn(new QPushButton(mCalibrationSettingsWidget)),

        mBottomText(new QLabel(this)){

    for(auto &x : mCurrCalibration) // default is to do nothing
        x = 0;

    setLayout(mLayout);

        mGeneralSettingsWidget->setTitle("General settings");
        mGeneralSettingsWidget->setStyleSheet("QGroupBox { font-weight: bold; }");
        mGeneralSettingsWidget->setLayout(mGeneralSettingsLayout);
        mGeneralSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            mNumThreadsWidget->setLayout(mNumThreadsLayout);

                mNumThreadsLabel->setText("Maximum number of threads: ");
                mNumThreadsSpinbox->setRange(1, QThread::idealThreadCount()*2);
                mNumThreadsSpinbox->setValue(QThread::idealThreadCount());
                mNumThreadsSpinbox->setMaximumSize(mNumThreadsSpinbox->size());

            mNumThreadsLayout->addWidget(mNumThreadsLabel);
            mNumThreadsLayout->addWidget(mNumThreadsSpinbox);

            mSpatialMaskWidget->setLayout(mSpatialMaskLayout);

                mSpatialMaskLabel->setText("Spatial mask: ");
                mSpatialMaskCurrLabel->setText("<b>(No mask set)</b>");
                mSpatialMaskSetBtn->setText("Set");
                connect(mSpatialMaskSetBtn, &QPushButton::clicked, this, &FileInputSettingsPanel::setImageMaskClick);
                mSpatialMaskClearBtn->setText("Clear");
                connect(mSpatialMaskClearBtn, &QPushButton::clicked, this, &FileInputSettingsPanel::clearImageMaskClick);

            mSpatialMaskLayout->addWidget(mSpatialMaskLabel);
            mSpatialMaskLayout->addWidget(mSpatialMaskCurrLabel);
            mSpatialMaskLayout->addWidget(mSpatialMaskSetBtn);
            mSpatialMaskLayout->addWidget(mSpatialMaskClearBtn);

        mGeneralSettingsLayout->addWidget(mNumThreadsWidget);
        mGeneralSettingsLayout->addWidget(mSpatialMaskWidget);

        mToTCorrectionSettingsWidget->setTitle("Time over Threshold Correction");
        mToTCorrectionSettingsWidget->setStyleSheet("QGroupBox { font-weight: bold; }");
        mToTCorrectionSettingsWidget->setLayout(mToTCorrectionSettingsLayout);
        mToTCorrectionSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            mToTCorrectionSourceWidget->setLayout(mToTCorrectionSourceLayout);

                mToTCorrLabel->setText("ToA Calibration: ");
                mToTCorrCurrLabel->setText("<b>(No calibration set)</b>");
                mToTCorrSetBtn->setText("Set");
                connect(mToTCorrSetBtn, &QPushButton::clicked, this, &FileInputSettingsPanel::setToACalibClick);
                mToTCorrClearBtn->setText("Clear");
                connect(mToTCorrClearBtn, &QPushButton::clicked, this, &FileInputSettingsPanel::clearToACalibClick);

            mToTCorrectionSourceLayout->addWidget(mToTCorrLabel);
            mToTCorrectionSourceLayout->addWidget(mToTCorrCurrLabel);
            mToTCorrectionSourceLayout->addWidget(mToTCorrSetBtn);
            mToTCorrectionSourceLayout->addWidget(mToTCorrClearBtn);

        mToTCorrectionSettingsLayout->addWidget(mToTCorrectionSourceWidget);

        mClusteringSettingsWidget->setTitle("Clustering");
        mClusteringSettingsWidget->setStyleSheet("QGroupBox { font-weight: bold; }");
        mClusteringSettingsWidget->setLayout(mClusteringSettingsLayout);
        mClusteringSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            mClusterWindowWidget->setLayout(mClusterWindowLayout);

                mClusterWindowXYLabel->setText("XY Half Window [pixels]: ");
                mClusterWindowXYEdit->setValidator(new QDoubleValidator(0.1, 128, 1, mClusterWindowXYEdit));
                mClusterWindowXYEdit->setText("5");
                mClusterWindowTLabel->setText("T Half Window [ns]: ");
                mClusterWindowTEdit->setValidator(new QDoubleValidator(1.5625, 15625, 2, mClusterWindowTEdit));
                mClusterWindowTEdit->setText("750");

            mClusterWindowLayout->addWidget(mClusterWindowXYLabel);
            mClusterWindowLayout->addWidget(mClusterWindowXYEdit);
            mClusterWindowLayout->addWidget(mClusterWindowTLabel);
            mClusterWindowLayout->addWidget(mClusterWindowTEdit);

            mMinClusterSizeWidget->setLayout(mMinClusterSizeLayout);

                mMinClusterSizeLabel->setText("Minimum number of counts in a cluster: ");
                mMinClusterSizeEdit->setRange(1, 99);
                mMinClusterSizeEdit->setValue(1);
                mMinClusterSizeEdit->setMaximumSize(mMinClusterSizeEdit->size());

            mMinClusterSizeLayout->addWidget(mMinClusterSizeLabel);
            mMinClusterSizeLayout->addWidget(mMinClusterSizeEdit);

        mClusteringSettingsLayout->addWidget(mClusterWindowWidget);
        mClusteringSettingsLayout->addWidget(mMinClusterSizeWidget);

        mBottomText->setText("Settings apply only to newly-imported files.");
        mBottomText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

        mCoincidenceSettingsWidget->setTitle("Coincidences");
        mCoincidenceSettingsWidget->setStyleSheet("QGroupBox { font-weight: bold; }");
        mCoincidenceSettingsWidget->setLayout(mCoincidenceSettingsLayout);
        mCoincidenceSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            mCoincidenceWindowWidget->setLayout(mCoincidenceWindowLayout);

                mCoincidenceWindowLabel->setText("Coincidence Window [ns]: ");
                mCoincidenceWindowEdit->setValidator(new QDoubleValidator(0.1, 10000, 1, mCoincidenceWindowEdit));
                mCoincidenceWindowEdit->setText("15");

            mCoincidenceWindowLayout->addWidget(mCoincidenceWindowLabel);
            mCoincidenceWindowLayout->addWidget(mCoincidenceWindowEdit);

        mCoincidenceSettingsLayout->addWidget(mCoincidenceWindowWidget);

        mCalibrationSettingsWidget->setTitle("Wavelength Calibration");
        mCalibrationSettingsWidget->setStyleSheet("QGroupBox { font-weight: bold; }");
        mCalibrationSettingsWidget->setLayout(mCalibrationSettingsLayout);
        mCalibrationSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            mCalibration1Widget->setLayout(mCalibration1Layout);

                mCalibrationSlope1Label->setText("Channel 1 Slope [nm/px]: ");
                mCalibrationSlope1Edit->setValidator(new QDoubleValidator(0.1, 10000, 1, mCoincidenceWindowEdit));
                mCalibrationSlope1Edit->setText("1");

                mCalibrationIntercept1Label->setText("Channel 1 Intercept [nm]: ");
                mCalibrationIntercept1Edit->setValidator(new QDoubleValidator(0.1, 10000, 1, mCoincidenceWindowEdit));
                mCalibrationIntercept1Edit->setText("0");

            mCalibration1Layout->addWidget(mCalibrationSlope1Label);
            mCalibration1Layout->addWidget(mCalibrationSlope1Edit);
            mCalibration1Layout->addWidget(mCalibrationIntercept1Label);
            mCalibration1Layout->addWidget(mCalibrationIntercept1Edit);

            mCalibration2Widget->setLayout(mCalibration2Layout);

                mCalibrationSlope2Label->setText("Channel 2 Slope [nm/px]: ");
                mCalibrationSlope2Edit->setValidator(new QDoubleValidator(0.1, 10000, 1, mCoincidenceWindowEdit));
                mCalibrationSlope2Edit->setText("1");

                mCalibrationIntercept2Label->setText("Channel 2 Intercept [nm]: ");
                mCalibrationIntercept2Edit->setValidator(new QDoubleValidator(0.1, 10000, 1, mCoincidenceWindowEdit));
                mCalibrationIntercept2Edit->setText("0");

            mCalibration2Layout->addWidget(mCalibrationSlope2Label);
            mCalibration2Layout->addWidget(mCalibrationSlope2Edit);
            mCalibration2Layout->addWidget(mCalibrationIntercept2Label);
            mCalibration2Layout->addWidget(mCalibrationIntercept2Edit);

            mLoadCalibrationBtn->setText("Load From File");
            connect(mLoadCalibrationBtn, &QPushButton::clicked, this, &FileInputSettingsPanel::loadCalibrationFileClick);

        mCalibrationSettingsLayout->addWidget(mCalibration1Widget);
        mCalibrationSettingsLayout->addWidget(mCalibration2Widget);
        mCalibrationSettingsLayout->addWidget(mLoadCalibrationBtn);

    mLayout->addWidget(mGeneralSettingsWidget);
    mLayout->addWidget(mToTCorrectionSettingsWidget);
    mLayout->addWidget(mClusteringSettingsWidget);
    mLayout->addWidget(mCoincidenceSettingsWidget);
    mLayout->addWidget(mCalibrationSettingsWidget);
    mLayout->addWidget(new QWidget());
    mLayout->addWidget(mBottomText);

    mCurrImageMask = std::make_unique<SpatialMask>(
                    false,
                    0, Tpx3Image::WIDTH,
                    0, Tpx3Image::WIDTH
            );

}

Tpx3ImportSettings FileInputSettingsPanel::getSettings() {

    int maxNumThreads = mNumThreadsSpinbox->value();

    SpatialMask mask;
    if (mCurrImageMask) {
        mask = *mCurrImageMask;
    } else {
        mask = {
            false,
            0, Tpx3Image::WIDTH,
            0, Tpx3Image::WIDTH
        };
    }

    float clusterWindowXY = std::stof(mClusterWindowXYEdit->text().toStdString());
    float clusterWindowT = std::stof(mClusterWindowTEdit->text().toStdString());
    int minClusterSize = mMinClusterSizeEdit->value();

    double coincidenceWindow = std::stod(mCoincidenceWindowEdit->text().toStdString());

    double ch1Slope = std::stod(mCalibrationSlope1Edit->text().toStdString());
    double ch1Intercept = std::stod(mCalibrationIntercept1Edit->text().toStdString());
    double ch2Slope = std::stod(mCalibrationSlope2Edit->text().toStdString());
    double ch2Intercept = std::stod(mCalibrationIntercept2Edit->text().toStdString());

    return {
        maxNumThreads,
        mask,

        mCurrCalibration,

        clusterWindowXY,
        clusterWindowT,
        minClusterSize,

        coincidenceWindow*1e-9,

        {
                ch1Slope,
                ch1Intercept,
                ch2Slope,
                ch2Intercept
        }
    };

}

void FileInputSettingsPanel::setImageMaskClick() {

    auto filename = QFileDialog::getOpenFileName(
            this,
            "Select Reference File",
            "./data",
            "Tpx3 Files (*.tpx3)"
    );

    if(filename.isEmpty())
        return;

    // we want to ignore any previously-set spatial mask
    auto import_settings = getSettings();
    import_settings.spatialMask = {
            false,
            0, Tpx3Image::WIDTH,
            0, Tpx3Image::WIDTH
    };
    import_settings.calibration = {
            1, 0,
            1, 0
    };

    mActions.lockUiForMasking->trigger();

    auto file_loader = new LoadRawFileThread(filename.toStdString(), import_settings, true);

    auto dialog = new QProgressDialog("Loading raw counts...", "Cancel", 0, 100, this);
    auto pbar = new FileImportProgressBar(dialog);
    dialog->setMinimumWidth(dialog->size().width()*1.5);
    dialog->setBar(pbar);
    dialog->setMinimumDuration(0);

    pbar->connectThread(file_loader);
    pbar->setIsLoading();

    connect(file_loader, &LoadRawFileThread::threadDone, [dialog, this](){
        dialog->close();
    });
    connect(file_loader, &LoadRawFileThread::yieldPixelData, this, &FileInputSettingsPanel::receiveImageMaskReference);

    QThreadPool::globalInstance()->start(file_loader);

}

void FileInputSettingsPanel::clearImageMaskClick() {

    mCurrImageMask.reset();
    mSpatialMaskCurrLabel->setText("<b>(No mask set)</b>");

}

void FileInputSettingsPanel::setToACalibClick() {

    auto filename = QFileDialog::getOpenFileName(
            this,
            "Select Calibration File",
            "./data",
            "ToA Calibration Files (*.txt)"
    );

    if(filename.isEmpty())
        return;

    // read the file
    std::ifstream input(filename.toStdString());
    std::string line;
    ToTCalibration result;
    for(auto &x : result)
        x = 0;

    while(std::getline(input, line)) {
        std::vector<std::string> row;
        std::stringstream row_stream(line);
        std::string elem;
        while(std::getline(row_stream, elem, ','))
            row.push_back(std::move(elem));

        if(row.size() != 2)
            throw std::runtime_error("Incorrect ToA calibration format.");

        unsigned ix = std::stoul(row[0]);
        double offset = std::stod(row[1]);

        if(ix >= result.size())
            throw std::runtime_error("Incorrect ToA calibration format.");

        result[ix] = -offset;
    }

    mCurrCalibration = result;

    auto new_label = "<b>(" + filename + ")</b>";
    mToTCorrCurrLabel->setText(new_label);

}

void FileInputSettingsPanel::clearToACalibClick() {

    // reset calibration
    for(auto &x : mCurrCalibration)
        x = 0;
    mToTCorrCurrLabel->setText("<b>(No calibration set)</b>");

}

void FileInputSettingsPanel::receiveImageMaskReference(Tpx3Image *image_ptr) {

    std::unique_ptr<Tpx3Image> image(image_ptr);

    if(!image) {
        mActions.unlockUi->trigger();
        return;
    }

    auto set_mask_dialog = new SetImageMaskDialog(std::move(image), this);
    connect(set_mask_dialog, &SetImageMaskDialog::yieldImageMask, this, &FileInputSettingsPanel::receiveImageMask);
    auto dialog_result = set_mask_dialog->exec();
    // regardless of the outcome, we need to re-enable UI
    mActions.unlockUi->trigger();

}

void FileInputSettingsPanel::receiveImageMask(spec_hom::SpatialMask mask, std::string filename) {

    mCurrImageMask = std::make_unique<SpatialMask>(mask);
    auto new_label = "<b>(" + filename + ")</b>";
    mSpatialMaskCurrLabel->setText(new_label.c_str());

}

// Simple linear regression
void linfit(const std::vector<double> &x, const std::vector<double> &y, double &m, double &b) {

    double x1 = 0, x2 = 0, y1 = 0, xy = 0;
    double n = x.size();
    assert(x.size() == y.size()); // vectors must be same length
    for(unsigned ix = 0; ix < x.size(); ++ix) {
        x1 += x[ix];
        x2 += x[ix]*x[ix];
        y1 += y[ix];
        xy += x[ix]*y[ix];
    }

    b = (xy*x1 - x2*y1)/(x1*x1 - n*x2);
    m = (y1 - n*b)/x1;

}

void FileInputSettingsPanel::loadCalibrationFileClick() {

    auto filename = QFileDialog::getOpenFileName(
            this,
            "Select Calibration File",
            "./data",
            "Wavelength Calibration Files (*.calib.csv)"
    );

    if(filename.isEmpty())
        return;

    // read the file
    std::ifstream input(filename.toStdString());
    std::string line;

    std::vector<double> channel1, channel2, wavelengths;

    std::getline(input, line); // skip header line
    while(std::getline(input, line)) {
        std::vector<std::string> row;
        std::stringstream row_stream(line);
        std::string elem;
        while(std::getline(row_stream, elem, ','))
            row.push_back(std::move(elem));

        if(row.size() != 3)
            throw std::runtime_error("Incorrect wavelength calibration format.");

        wavelengths.push_back(std::stod(row[0]));
        channel1.push_back(std::stod(row[1]));
        channel2.push_back(std::stod(row[2]));
    }

    double slope1, intercept1, slope2, intercept2;
    linfit(channel1, wavelengths, slope1, intercept1);
    linfit(channel2, wavelengths, slope2, intercept2);

    mCalibrationSlope1Edit->setText(QString::number(slope1));
    mCalibrationIntercept1Edit->setText(QString::number(intercept1));
    mCalibrationSlope2Edit->setText(QString::number(slope2));
    mCalibrationIntercept2Edit->setText(QString::number(intercept2));

}