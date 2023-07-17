#include "fileview.h"

#include "qcustomplot.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

ToTDistributionView::ToTDistributionView(QWidget *parent, Tpx3Image *image) :
    QWidget(parent),
    mLayout(new QVBoxLayout(this)),
    mPlot(new QCustomPlot(this)) {

    setLayout(mLayout);

    auto hist_pair = image->toTDistribution();
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

    mPlot->xAxis->setLabel("Time over threshold [ns]");
    mPlot->yAxis->setLabel("Counts");

    mPlot->replot();

    mLayout->addWidget(mPlot);

}