#include "fileview.h"

#include <iostream>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

RawImageView::RawImageView(QWidget *parent, Tpx3Image *src) :
    Hist2DView(parent, src) {

    title("Distribution of Raw Counts");
    xLabel("Camera X");
    yLabel("Camera Y");
    colorbarLabel("Raw Counts per Pixel");

    auto image_2d = source()->rawPacketImage();
    auto &data_arr = data();

    for (int x=0; x<Tpx3Image::WIDTH; ++x) {
        data_arr.emplace_back(Tpx3Image::HEIGHT);
        for (int y=0; y<Tpx3Image::HEIGHT; ++y) {
            data_arr[x][y] = image_2d[x][y];
        }
    }

    updatePlot();

}