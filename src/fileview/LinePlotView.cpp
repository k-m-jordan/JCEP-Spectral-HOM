#include "fileview.h"

#include <iostream>
#include <fstream>

using namespace spec_hom;

LinePlotView::LinePlotView(QWidget *parent, const Tpx3Image *src) :
        FileViewPanel(parent, src),
        mXData(),
        mYData(),
        mYErr(),
        mTitle(),
        mXLabel("X"),
        mYLabel("Y") {

    // Do nothing

}

void LinePlotView::saveDataBtnClick() {

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

    auto &x_data = xData(), &y_data = yData(), &y_err = yErr();
    bool has_err = !y_err.isEmpty();

    std::ofstream output(filename.toStdString());

    // header
    output << mXLabel.toStdString() << ", " << mYLabel.toStdString();
    if(has_err)
        output << ", " << "Error Bar";
    output << std::endl;

    for(auto ix = 0; ix < x_data.size(); ++ix) {
        output << x_data[ix] << ", " << y_data[ix];
        if(has_err)
            output << ", " << y_err[ix];
        output << std::endl;
    }

    output.close();

}

void LinePlotView::updatePlot() {

    plot()->clearGraphs();
    plot()->clearItems();

    auto max_y = *std::max_element(mYData.cbegin(), mYData.cend());
    auto min_y = *std::min_element(mYData.cbegin(), mYData.cend());

    plot()->addGraph()->setData(mXData, mYData);

    plot()->plotLayout()->insertRow(0);

    QFont f;
    f.setPointSize(12);
    plot()->plotLayout()->addElement(0, 0, new QCPTextElement(plot(), mTitle, f));

    plot()->xAxis->setRange(mXData.front(), mXData.back());
    plot()->yAxis->setRange(min_y, max_y);

    plot()->xAxis->setLabel(mXLabel);
    plot()->yAxis->setLabel(mYLabel);

    if(!mYErr.isEmpty()) {
        auto error_bars = new QCPErrorBars(plot()->xAxis, plot()->yAxis);
        error_bars->setAntialiased(false);
        error_bars->setDataPlottable(plot()->graph(0));
        error_bars->setPen(QPen(QColor(230, 230, 230)));
        error_bars->setData(mYErr);
    }

    plot()->replot();

}