#include "threadutils.h"

#include "ui.h"

using namespace spec_hom;

BgThread::BgThread() :
    QObject(),
    QRunnable(),
    mShouldCancel(false) {

    // Do nothing

}

void BgThread::run() {

    execute();
    emit threadDone();

}

bool BgThread::shouldCancel() const {

    return mShouldCancel;

}

void BgThread::cancel() {

    mShouldCancel = true;

}