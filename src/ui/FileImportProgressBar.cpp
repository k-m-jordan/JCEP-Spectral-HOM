#include "ui.h"

#include <iostream>

#include "tpx3/tpx3.h"
#include "threadutils.h"

using namespace spec_hom;

FileImportProgressBar::FileImportProgressBar(QWidget *parent) :
        QProgressBar(parent),
        mConnectedThread(nullptr),
        mIsQueued(false),
        mIsLoading(false),
        mIsLoaded(false) {

    setRange(0,100);
    setValue(0);
    setTextVisible(true);
    setAlignment(Qt::AlignCenter);

    QFont bold_font;
    bold_font.setBold(true);
    setFont(bold_font);

}

QString FileImportProgressBar::text() const {

    if(minimum() == maximum())
        return format();
    else
        return QProgressBar::text();

}

void FileImportProgressBar::setIndefinite(bool value) {

    if(value) {
        setRange(0,0);
    } else {
        setRange(0,100);
    }

}

std::string FileImportProgressBar::label() const {

    return format().toStdString();

}

QColor FileImportProgressBar::labelColor() const {

    return palette().color(QPalette::Text);

}

FileImportProgressBar* FileImportProgressBar::clone(QWidget *parent) const {

    auto new_bar = new FileImportProgressBar(parent);
    new_bar->setLabel(label());
    new_bar->setLabelColor(labelColor());
    new_bar->setRange(minimum(), maximum());
    new_bar->mIsQueued = mIsQueued;
    new_bar->mIsLoading = mIsLoading;
    new_bar->mIsLoaded = mIsLoaded;
    new_bar->connectThread(this);
    return new_bar;

}

void FileImportProgressBar::setLabel(std::string txt) {

    setFormat(txt.c_str());

}

void FileImportProgressBar::setLabelColor(QColor color) {

    QPalette p = palette();
    p.setColor(QPalette::Text, color);
    setPalette(p);

}

void FileImportProgressBar::connectThread(LoadRawFileThread *thread) {

    mConnectedThread = thread;

    if(!thread)
        return;

    connect(thread, &LoadRawFileThread::setProgress, this, &FileImportProgressBar::setValue);
    connect(thread, &LoadRawFileThread::setProgressIndefinite, this, &FileImportProgressBar::setIndefinite);
    connect(thread, &LoadRawFileThread::setProgressText, this, &FileImportProgressBar::setLabel);
    connect(thread, &LoadRawFileThread::setProgressTextColor, this, &FileImportProgressBar::setLabelColor);
    connect(thread, &LoadRawFileThread::threadDone, this, &FileImportProgressBar::disconnectThread);

}

void FileImportProgressBar::connectThread(const FileImportProgressBar *other) {

    connectThread(other->mConnectedThread);

}

void FileImportProgressBar::disconnectThread() {

    mConnectedThread = nullptr;
    mIsLoading = false;

}

void FileImportProgressBar::setQueued() {

    setLabel("Queued");
    setLabelColor(QColor(242, 151, 39));
    setIndefinite(false);
    setValue(0);

    mIsQueued = true;
    mIsLoaded = false;

}

void FileImportProgressBar::setLoaded() {

    setLabel("Imported");
    setLabelColor(QColor(0x02, 0x8A, 0x0F));
    setIndefinite(false);
    setValue(0);

    mIsQueued = false;
    mIsLoading = false;
    mIsLoaded = true;

}

void FileImportProgressBar::setIsLoading() {

    mIsLoading = true;
    mIsLoaded = false;

}