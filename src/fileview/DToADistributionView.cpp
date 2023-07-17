#include "fileview.h"

#include <fstream>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

DToADistributionView::DToADistributionView(QWidget *parent, Tpx3Image *image) :
        QWidget(parent),
        mXData(),
        mYData(),
        mLayout(new QVBoxLayout(this)),
        mPlot(new QCustomPlot(this)),
        mExportCalibButton(new QPushButton(this)) {

    setLayout(mLayout);

    QVector<double> y_err;
    std::tie(mXData, mYData, y_err) = image->dToTDistribution();

    auto max_y = *std::max_element(mYData.cbegin(), mYData.cend());
    auto min_y = *std::min_element(mYData.cbegin(), mYData.cend());

    mPlot->addGraph()->setData(mXData, mYData);

    mPlot->xAxis->setRange(mXData.front(), mXData.back());
    mPlot->yAxis->setRange(min_y, max_y);

    mPlot->xAxis->setLabel("Time over Threshold [ns]");
    mPlot->yAxis->setLabel("Delay in Time of Arrival");

    auto error_bars = new QCPErrorBars(mPlot->xAxis, mPlot->yAxis);
    error_bars->setAntialiased(false);
    error_bars->setDataPlottable(mPlot->graph(0));
    error_bars->setPen(QPen(QColor(180, 180, 180)));
    error_bars->setData(y_err);

    mPlot->replot();

    mExportCalibButton->setText("Export Data as ToT Calibration");
    connect(mExportCalibButton, &QPushButton::clicked, this, &DToADistributionView::exportCalibrationClicked);

    mLayout->addWidget(mPlot);
    mLayout->addWidget(mExportCalibButton);

}

void DToADistributionView::exportCalibrationClicked() {

    auto filename = QFileDialog::getSaveFileName(
            this,
            "Select Reference File",
            "./data",
            "ToT Calibration Files (*.txt)"
    );

    if(filename.isEmpty())
        return;

    std::ofstream output(filename.toStdString());
    for(auto ix = 0; ix < mXData.size(); ++ix) {
        output << (mXData[ix] / TOT_UNIT_SIZE) << "," << mYData[ix] << std::endl;
    }

    output.close();

}