#include "fileview.h"

#include "qcustomplot.h"

using namespace spec_hom;

FileViewPanel::FileViewPanel(QWidget *parent, const Tpx3Image *src) :
    QWidget(parent),
    mSrcImage(src),

    mLayout(new QVBoxLayout(this)),
    mPlot(new QCustomPlot(this)),
    mToolbar(new QWidget(this)),
    mToolbarLayout(new QHBoxLayout(mToolbar)),

    mSaveImageBtn(new QPushButton(this)),
    mExportDataBtn(new QPushButton(this)) {

    if(!src)
        throw std::runtime_error("Attempt to initialize a FileViewPanel without a source image");

    setLayout(mLayout);

        mToolbar->setLayout(mToolbarLayout);
        mToolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            mSaveImageBtn->setText("Save Image");
            mSaveImageBtn->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveButton));
            mSaveImageBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
            connect(mSaveImageBtn, &QPushButton::clicked, this, &FileViewPanel::saveImageBtnClick);

            mExportDataBtn->setText("Export Data as CSV");
            mExportDataBtn->setIcon(this->style()->standardIcon(QStyle::SP_FileIcon));
            mExportDataBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
            connect(mExportDataBtn, &QPushButton::clicked, this, &FileViewPanel::saveDataBtnClick);

        mToolbarLayout->addWidget(mSaveImageBtn);
        mToolbarLayout->addWidget(mExportDataBtn);

    mLayout->addWidget(mPlot);
    mLayout->addWidget(mToolbar);

}

void FileViewPanel::saveImageBtnClick() {

    auto filename = QFileDialog::getSaveFileName(
            this,
            "Save Image File",
            "./data",
            "Image (*.png)"
    );

    if(filename.isEmpty())
        return;

    if(!filename.toLower().endsWith(".png"))
        filename += ".png";

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Failed to write to file");
    } else {
        plot()->savePng(filename);
    }

}