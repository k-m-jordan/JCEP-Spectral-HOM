#include "fileview.h"

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

StartStopHistogramView::StartStopHistogramView(QWidget *parent, Tpx3Image *image) :
        LinePlotView (parent, image) {

    std::tie(xData(), yData()) = source()->startStopHistogram(MIN_TICK, 128);

    title("Histogram of Start-Stop Intervals");
    xLabel("Interval between clicks [sec]");
    yLabel("Number of Occurrences");

    updatePlot();

}