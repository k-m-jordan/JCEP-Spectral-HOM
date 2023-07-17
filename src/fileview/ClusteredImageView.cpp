#include "fileview.h"

#include <iostream>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

ClusteredImageView::ClusteredImageView(QWidget *parent, Tpx3Image *src) :
        QWidget(parent),
        mLayout(new QVBoxLayout(this)),
        mPlot(new QCustomPlot(this)) {

    setLayout(mLayout);

    mPlot->axisRect()->setupFullAxesBox(true);
    mPlot->xAxis->setLabel("Camera X");
    mPlot->yAxis->setLabel("Camera Y");

    auto width = Tpx3Image::WIDTH, height = Tpx3Image::HEIGHT;

    auto *colorMap = new QCPColorMap(mPlot->xAxis, mPlot->yAxis);
    colorMap->data()->setSize(width, height);
    colorMap->data()->setRange({0, static_cast<double>(width)-1},
                               {0, static_cast<double>(height)-1});

    auto image_2d = src->clusterImage();

    for (int x=0; x<Tpx3Image::WIDTH; ++x) {
        for (int y=0; y<Tpx3Image::HEIGHT; ++y) {
            colorMap->data()->setCell(x, y, image_2d[x][y]);
        }
    }

    auto *colorScale = new QCPColorScale(mPlot);
    mPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Cluster Centroids per Pixel");

    colorMap->setGradient(QCPColorGradient::gpThermal);
    colorMap->rescaleDataRange();
    auto *marginGroup = new QCPMarginGroup(mPlot);
    mPlot->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorMap->setAntialiased(true);
    mPlot->rescaleAxes();

    mLayout->addWidget(mPlot);

}