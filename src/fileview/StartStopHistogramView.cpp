#include "fileview.h"

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

StartStopHistogramView::StartStopHistogramView(QWidget *parent, Tpx3Image *image) :
        QWidget(parent),
        mLayout(new QVBoxLayout(this)),
        mPlot(new QCustomPlot(this)) {

    setLayout(mLayout);

    auto hist_pair = image->startStopHistogram(MIN_TICK, 128);
    auto &x = hist_pair.first;
    auto &y = hist_pair.second;

    double max_y = 0;
    for(auto y_i : y) {
        if (y_i > max_y)
            max_y = y_i;
    }

    mPlot->addGraph()->setData(x, y);

    mPlot->xAxis->setRange(x.front(), x.back());
    mPlot->yAxis->setRange(0, max_y);

    mPlot->xAxis->setLabel("Interval between clicks [sec]");
    mPlot->yAxis->setLabel("Number of Occurrences");

    mPlot->replot();

    mLayout->addWidget(mPlot);

}