#include "fileview.h"

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

ToTDistributionView::ToTDistributionView(QWidget *parent, Tpx3Image *image) :
        LinePlotView (parent, image) {

    std::tie(xData(),yData()) = source()->toTDistribution();

    title("Histogram of Time over Threshold Values");
    xLabel("Time over threshold [ns]");
    yLabel("Counts");

    updatePlot();

}