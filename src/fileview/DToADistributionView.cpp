#include "fileview.h"

#include <fstream>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

DToADistributionView::DToADistributionView(QWidget *parent, Tpx3Image *image) :
        LinePlotView(parent, image),
        mExportCalibButton(new QPushButton(this)) {

    auto &x_data = xData(), &y_data = yData(), &y_err = yErr();

    std::tie(x_data, y_data, y_err) = source()->dToADistribution(4);

    title("Mean Delay vs. Time over Threshold");
    xLabel("Time over Threshold [ns]");
    yLabel("Delay in Time of Arrival [ns]");

    updatePlot();

    mExportCalibButton->setText("Export Data as ToA Calibration");
    mExportCalibButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    mExportCalibButton->setIcon(this->style()->standardIcon(QStyle::SP_ArrowForward));
    connect(mExportCalibButton, &QPushButton::clicked, this, &DToADistributionView::exportCalibrationClicked);

    toolbarLayout()->addWidget(mExportCalibButton);

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

    if(!filename.toLower().endsWith(".txt"))
        filename += ".txt";

    auto &x_data = xData(), &y_data = yData();

    std::ofstream output(filename.toStdString());
    for(auto ix = 0; ix < x_data.size(); ++ix) {
        output << (x_data[ix] / TOT_UNIT_SIZE) << "," << y_data[ix] << std::endl;
    }

    output.close();

}