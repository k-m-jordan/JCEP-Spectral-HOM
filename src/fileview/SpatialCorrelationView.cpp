#include "fileview.h"

#include <iostream>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

SpatialCorrelationView::SpatialCorrelationView(QWidget *parent, Tpx3Image *src) :
        Hist2DView(parent, src) {

    title("Spatial Correlations (X Axis)");
    xLabel("X1");
    yLabel("X2");
    colorbarLabel("Counts Per Bin");

    auto size = Tpx3Image::SPATIAL_CORR_SIZE;

    auto image_2d = source()->spatialCorrelations();
    auto &data_arr = data();

    for (int x=0; x<size; ++x) {
        data_arr.emplace_back(size);
        for (int y=0; y<size; ++y) {
            data_arr[x][y] = image_2d[x][y];
        }
    }

    updatePlot();

}