#include "fileview.h"

#include <iostream>
#include <fstream>

using namespace spec_hom;

Hist2DView::Hist2DView(QWidget *parent, const Tpx3Image *src) :
        FileViewPanel(parent, src),
        mTitle(),
        mXLabel("X"),
        mYLabel("Y"),
        mColorbarLabel() {

    // Do nothing

}

void Hist2DView::saveDataBtnClick() {

    auto filename = QFileDialog::getSaveFileName(
            this,
            "Save Plot as CSV",
            "./data",
            "CSV Files (*.csv)"
    );

    if(filename.isEmpty())
        return;

    if(!filename.toLower().endsWith(".csv"))
        filename += ".csv";

    auto &data_arr = data();

    std::ofstream output(filename.toStdString());

    // header
    output << mXLabel.toStdString() << ", " << mYLabel.toStdString() << ", " << mColorbarLabel.toStdString();
    output << std::endl;

    for(auto r = 0; r < data_arr.size(); ++r) {
        for(auto c = 0; c < data_arr[r].size(); ++c) {
            output << r << ", " << c << ", " << data_arr[r][c] << std::endl;
        }
    }

    output.close();

}

void Hist2DView::updatePlot() {

    plot()->clearItems();
    plot()->clearGraphs();

    plot()->axisRect()->setupFullAxesBox(true);

    plot()->xAxis->setLabel(mXLabel);
    plot()->yAxis->setLabel(mYLabel);

    auto width = mData.size(), height = mData[0].size();

    auto *colorMap = new QCPColorMap(plot()->xAxis, plot()->yAxis);
    colorMap->data()->setSize(width, height);
    colorMap->data()->setRange({0, static_cast<double>(width)-1},
                               {0, static_cast<double>(height)-1});

    for (int x=0; x<width; ++x) {
        for (int y=0; y<height; ++y) {
            colorMap->data()->setCell(x, y, mData[x][y]);
        }
    }

    plot()->plotLayout()->insertRow(0);

    QFont f;
    f.setPointSize(12);
    plot()->plotLayout()->addElement(0, 0, new QCPTextElement(plot(), mTitle, f));

    auto *colorScale = new QCPColorScale(plot());
    plot()->plotLayout()->addElement(1, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel(mColorbarLabel);

    colorMap->setGradient(QCPColorGradient::gpThermal);
    colorMap->rescaleDataRange();
    auto *marginGroup = new QCPMarginGroup(plot());
    plot()->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorMap->setAntialiased(true);
    plot()->rescaleAxes();

}