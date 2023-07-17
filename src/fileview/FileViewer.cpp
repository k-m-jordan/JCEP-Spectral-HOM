#include "fileview.h"

#include "tpx3/tpx3.h"

using namespace spec_hom;

enum ViewType {
    VIEWTYPE_RAW_IMAGE = 0,
    VIEWTYPE_TOT_DISTRIBUTION,
    VIEWTYPE_CLUSTERED_IMAGE,
    VIEWTYPE_START_STOP_HISTOGRAM,
    VIEWTYPE_DTOA_DISTRIBUTION,
    VIEWTYPE_NUM
};

FileViewer::FileViewer(QWidget *parent, Tpx3Image *image) :
        QWidget(parent),
        mImage(image),

        mLayout(new QVBoxLayout(this)),
        mCenterWidget(new QGroupBox(this)),
        mCenterLayout(new QVBoxLayout(mCenterWidget)),
        mViewSelection(new QComboBox(mCenterWidget)),
        mViewContainer(new QWidget(mCenterWidget)) {

    setLayout(mLayout);

        mCenterWidget->setTitle(fullFilename().c_str());
        mCenterWidget->setStyleSheet("QGroupBox { font-weight: bold; }");
        mCenterWidget->setLayout(mCenterLayout);

        mCenterLayout->addWidget(mViewSelection);
        mCenterLayout->addWidget(mViewContainer, true);

    mLayout->addWidget(mCenterWidget);

    // setup signals
    connect(mViewSelection, &QComboBox::currentIndexChanged, this, &FileViewer::viewTypeChanged);

    // setup combo box options
    mViewSelection->addItem("Raw Image");
    mViewSelection->addItem("Time over Threshold Distribution");
    mViewSelection->addItem("Clustered Image");
    mViewSelection->addItem("Start-Stop Histogram");
    mViewSelection->addItem("Relative Time of Arrival Distribution");

    mViewSelection->setCurrentIndex(VIEWTYPE_RAW_IMAGE);

}

const std::string& FileViewer::fullFilename() const {

    return mImage->fullFilename();

}

void FileViewer::viewTypeChanged(int newIndex) {

    // we need to delete the current view and create a new one
    auto oldViewContainer = mViewContainer;
    mViewContainer = new QWidget(mCenterWidget);
    auto oldLayoutItem = mCenterLayout->replaceWidget(oldViewContainer, mViewContainer);
    delete oldLayoutItem->widget();
    delete oldLayoutItem;

    auto *viewContainerLayout = new QVBoxLayout(mViewContainer);
    mViewContainer->setLayout(viewContainerLayout);

    switch(newIndex) {
        case VIEWTYPE_RAW_IMAGE:
            viewContainerLayout->addWidget(new RawImageView(mViewContainer, mImage));
            break;
        case VIEWTYPE_TOT_DISTRIBUTION:
            viewContainerLayout->addWidget(new ToTDistributionView(mViewContainer, mImage));
            break;
        case VIEWTYPE_CLUSTERED_IMAGE:
            viewContainerLayout->addWidget(new ClusteredImageView(mViewContainer, mImage));
            break;
        case VIEWTYPE_START_STOP_HISTOGRAM:
            viewContainerLayout->addWidget(new StartStopHistogramView(mViewContainer, mImage));
            break;
        case VIEWTYPE_DTOA_DISTRIBUTION:
            viewContainerLayout->addWidget(new DToADistributionView(mViewContainer, mImage));
            break;
        default:
            assert(0); // one of the menu items is not implemented!
    }

}