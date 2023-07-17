#include "ui.h"

#include <algorithm>
#include <vector>
#include <iostream>
#include <cmath>

#include "qcustomplot.h"

constexpr int SLIDER_RES = 100;

using namespace spec_hom;

SetImageMaskDialog::SetImageMaskDialog(std::unique_ptr<Tpx3Image> &&reference, QWidget *parent) :
        QDialog(parent),
        mRefImage(std::move(reference)),
        mRawImage(mRefImage->rawPacketImage()),
        mLastFit(false, -1, -1, -1, -1),

        mLayout(new QHBoxLayout(this)),

        mLeftWidget(new QWidget(this)),
        mLeftLayout(new QVBoxLayout(mLeftWidget)),
        mImagePlot(new QCustomPlot(mLeftWidget)),

        mRightWidget(new QWidget(this)),
        mRightLayout(new QVBoxLayout(mRightWidget)),

        mFitPlot(new QCustomPlot(mRightWidget)),

        mLineDirWidget(new QWidget(mRightWidget)),
        mLineDirLayout(new QHBoxLayout(mLineDirWidget)),
        mHorButton(new QRadioButton(mLineDirWidget)),
        mVertButton(new QRadioButton(mLineDirWidget)),

        mSigmaWidget(new QWidget(mRightWidget)),
        mSigmaLayout(new QHBoxLayout(mSigmaWidget)),
        mSigmaLabel(new QLabel(mSigmaWidget)),
        mSigmaSlider(new QSlider(mSigmaWidget)),

        mAcceptButton(new QPushButton(mRightWidget)) {

    assert(mRefImage);

    setWindowTitle("Pixel Mask Selection");
    setLayout(mLayout);

    mLeftWidget->setLayout(mLeftLayout);

        mImagePlot->axisRect()->setupFullAxesBox(true);
        mImagePlot->xAxis->setLabel("Camera X");
        mImagePlot->yAxis->setLabel("Camera Y");

        auto width = Tpx3Image::WIDTH, height = Tpx3Image::HEIGHT;

        auto *colorMap = new QCPColorMap(mImagePlot->xAxis, mImagePlot->yAxis);
        colorMap->data()->setSize(width, height);
        colorMap->data()->setRange({0, static_cast<double>(width)-1},
                                   {0, static_cast<double>(height)-1});

        auto image_2d = mRefImage->rawPacketImage();

        for (int x=0; x<Tpx3Image::WIDTH; ++x) {
            for (int y=0; y<Tpx3Image::HEIGHT; ++y) {
                colorMap->data()->setCell(x, y, image_2d[x][y]);
            }
        }

        colorMap->setGradient(QCPColorGradient::gpThermal);
        colorMap->rescaleDataRange();
        mImagePlot->rescaleAxes();
        colorMap->setAntialiased(false);

    mLeftLayout->addWidget(mImagePlot);

    mRightWidget->setLayout(mRightLayout);

        mFitPlot->plotLayout()->insertRow(0);
        mFitPlot->plotLayout()->addElement(0, 0, new QCPTextElement(mFitPlot, ""));

        mLineDirWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
        mLineDirWidget->setLayout(mLineDirLayout);

            mHorButton->setText("Horizontal Lines");
            mHorButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
            mHorButton->setStyleSheet("QRadioButton {text-align: center;}");
            connect(mHorButton, &QRadioButton::clicked, this, &SetImageMaskDialog::updateSlicePlot);
            mVertButton->setText("Vertical Lines");
            mVertButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
            mVertButton->setStyleSheet("QRadioButton {text-align: center;}");
            connect(mVertButton, &QRadioButton::clicked, this, &SetImageMaskDialog::updateSlicePlot);

        mLineDirLayout->addWidget(mHorButton);
        mLineDirLayout->addWidget(mVertButton);

        mSigmaWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
        mSigmaWidget->setLayout(mSigmaLayout);

            mSigmaLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
            mSigmaLabel->setText("Mask width (+/-): ");
            mSigmaSlider->setOrientation(Qt::Horizontal);
            mSigmaSlider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
            mSigmaSlider->setRange(1, 12*SLIDER_RES);
            mSigmaSlider->setValue(4*SLIDER_RES);
            connect(mSigmaSlider, &QSlider::valueChanged, this, &SetImageMaskDialog::updateSlicePlot);

        mSigmaLayout->addWidget(mSigmaLabel);
        mSigmaLayout->addWidget(mSigmaSlider);

        mAcceptButton->setText("Accept");
        mAcceptButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
        connect(mAcceptButton, &QPushButton::clicked, this, &SetImageMaskDialog::acceptClicked);

    mRightLayout->addWidget(mFitPlot);
    mRightLayout->addWidget(mLineDirWidget);
    mRightLayout->addWidget(mSigmaWidget);
    mRightLayout->addWidget(mAcceptButton);

    mLayout->addWidget(mLeftWidget);
    mLayout->addWidget(mRightWidget);

    // Look for peaks along horizontal and vertical direction, and set option accordingly
    std::vector<unsigned> h_slice, v_slice;
    h_slice.reserve(Tpx3Image::HEIGHT);
    v_slice.reserve(Tpx3Image::WIDTH);

    for (unsigned r = 0; r < Tpx3Image::HEIGHT; ++r) {
        unsigned slice_tot = 0;
        for (unsigned c = 0; c < Tpx3Image::WIDTH; ++c) {
            slice_tot += mRawImage[r][c];
        }
        v_slice.push_back(slice_tot);
    }
    for (unsigned c = 0; c < Tpx3Image::WIDTH; ++c) {
        unsigned slice_tot = 0;
        for (unsigned r = 0; r < Tpx3Image::HEIGHT; ++r) {
            slice_tot += mRawImage[r][c];
        }
        h_slice.push_back(slice_tot);
    }

    auto h_slice_max = *std::max_element(h_slice.cbegin(), h_slice.cend());
    auto v_slice_max = *std::max_element(v_slice.cbegin(), v_slice.cend());

    if(h_slice_max > v_slice_max)
        mHorButton->click();
    else
        mVertButton->click();

    // Make "Accept" button default response to Enter key
    setTabOrder(mAcceptButton, mHorButton);
    setTabOrder(mAcceptButton, mVertButton);

    setFixedSize(width*5, height*2.5);
    show();

}

void SetImageMaskDialog::updateSlicePlot() {

    bool h_lines;
    if (mHorButton->isChecked())
        h_lines = true;
    else if (mVertButton->isChecked())
        h_lines = false;
    else
        throw std::runtime_error("SetImageMaskDialog::updateSlicePlot(): No line direction chosen.");

    // We need to fit the data
    QVector<double> x, y, fit_y;
    mLastFit = LinePair::find(mRawImage, h_lines, &x, &y, &fit_y);
    auto &lines = mLastFit;

    auto max_y = std::max_element(y.cbegin(), y.cend());

    mFitPlot->clearGraphs();

    mFitPlot->plotLayout()->removeAt(mFitPlot->plotLayout()->rowColToIndex(0,0));
    if(h_lines)
        mFitPlot->plotLayout()->addElement(0, 0, new QCPTextElement(mFitPlot, "Integration Along Rows"));
    else
        mFitPlot->plotLayout()->addElement(0, 0, new QCPTextElement(mFitPlot, "Integration Along Columns"));

    mFitPlot->addGraph()->setData(x, y);
    mFitPlot->addGraph()->setData(x, fit_y);
    mFitPlot->xAxis->setLabel("Pixel");
    mFitPlot->yAxis->setLabel("Integrated Counts");
    mFitPlot->xAxis->setRange(1, TPX3_SENSOR_SIZE);
    mFitPlot->yAxis->setRange(0, *max_y);

    mFitPlot->replot();

    // Draw rectangles on plot

    mImagePlot->clearItems(); // deletes mRect1, mRect2, mRect3

    QColor rect_color("red");
    rect_color.setAlpha(150);

    std::vector<QCPItemRect*> rects(3);
    for(auto &rect : rects) {
        rect = new QCPItemRect(mImagePlot);

        rect->setBrush(QBrush(rect_color));
        rect->setClipAxisRect(mImagePlot->axisRect());
        rect->topLeft->setAxes(mImagePlot->axisRect()->axis(QCPAxis::atBottom),
                               mImagePlot->axisRect()->axis(QCPAxis::atLeft));
        rect->bottomRight->setAxes(mImagePlot->axisRect()->axis(QCPAxis::atBottom),
                                   mImagePlot->axisRect()->axis(QCPAxis::atLeft));
    }

    auto num_sigma = static_cast<double>(mSigmaSlider->value()) / SLIDER_RES;
    std::ostringstream num_sigma_text;
    num_sigma_text.precision(1);
    num_sigma_text << std::fixed << num_sigma;
    auto new_sigma_text = "Mask width (+/-): " + num_sigma_text.str() + " sigma";
    mSigmaLabel->setText(new_sigma_text.c_str());

    double bottom_rect_top, top_rect_bottom, mid_rect_top, mid_rect_bottom;
    lines.getRectBounds(bottom_rect_top, mid_rect_bottom, mid_rect_top, top_rect_bottom, num_sigma);
    bottom_rect_top /= PIXEL_SIZE;
    top_rect_bottom /= PIXEL_SIZE;
    mid_rect_top /= PIXEL_SIZE;
    mid_rect_bottom /= PIXEL_SIZE;

    if (h_lines) {
        rects[0]->topLeft->setCoords(QPointF(0, bottom_rect_top));
        rects[0]->bottomRight->setCoords(QPointF(Tpx3Image::WIDTH, 0));
        rects[1]->topLeft->setCoords(QPointF(0, mid_rect_top));
        rects[1]->bottomRight->setCoords(QPointF(Tpx3Image::WIDTH, mid_rect_bottom));
        rects[2]->topLeft->setCoords(QPointF(0, Tpx3Image::HEIGHT));
        rects[2]->bottomRight->setCoords(QPointF(Tpx3Image::WIDTH, top_rect_bottom));
    } else {
        rects[0]->topLeft->setCoords(QPointF(0, Tpx3Image::HEIGHT));
        rects[0]->bottomRight->setCoords(QPointF(bottom_rect_top, 0));
        rects[1]->topLeft->setCoords(QPointF(mid_rect_bottom, Tpx3Image::HEIGHT));
        rects[1]->bottomRight->setCoords(QPointF(mid_rect_top, 0));
        rects[2]->topLeft->setCoords(QPointF(top_rect_bottom, Tpx3Image::HEIGHT));
        rects[2]->bottomRight->setCoords(QPointF(Tpx3Image::WIDTH, 0));
    }

    rects[1]->setVisible(mid_rect_top > mid_rect_bottom);

    mImagePlot->replot();

}

void SetImageMaskDialog::acceptClicked() {

    double mask_min_1, mask_max_1, mask_min_2, mask_max_2;
    mLastFit.getRectBounds(mask_min_1, mask_max_1, mask_min_2, mask_max_2, static_cast<double>(mSigmaSlider->value())/SLIDER_RES);

    int imin1 = static_cast<int>(mask_min_1 / PIXEL_SIZE),
            imax1 = static_cast<int>(mask_max_1 / PIXEL_SIZE),
            imin2 = static_cast<int>(mask_min_2 / PIXEL_SIZE),
            imax2 = static_cast<int>(mask_max_2 / PIXEL_SIZE);

    SpatialMask mask {
        mVertButton->isChecked(),
        imin1, imax1,
        imin2, imax2
    };

    emit yieldImageMask(mask, mRefImage->fullFilename());

    accept();

}