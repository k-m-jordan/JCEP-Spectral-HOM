#include "fileview.h"

#include <iostream>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

ClusteredImageView::ClusteredImageView(QWidget *parent, Tpx3Image *src) :
        Hist2DView(parent, src) {

    title("Distribution of Cluster Centroids");
    xLabel("Camera X");
    yLabel("Camera Y");
    colorbarLabel("Cluster Centroids per Pixel");

    auto width = Tpx3Image::WIDTH, height = Tpx3Image::HEIGHT;

    auto image_2d = source()->clusterImage();
    auto &data_arr = data();

    for (int x=0; x<Tpx3Image::WIDTH; ++x) {
        data_arr.emplace_back(height);
        for (int y=0; y<Tpx3Image::HEIGHT; ++y) {
            data_arr[x][y] = image_2d[x][y];
        }
    }

    updatePlot();

}