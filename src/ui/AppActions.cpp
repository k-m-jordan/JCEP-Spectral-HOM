#include "ui.h"

using namespace spec_hom;

AppActions::AppActions(QWidget *parent) :
    close(new QAction(parent)),
    openFilesDialog(new QAction(parent)),
    startImportFiles(new QAction(parent)),
    stopImportFiles(new QAction(parent)),
    clearFileList(new QAction(parent)),
    lockUiForImporting(new QAction(parent)),
    doneImporting(new QAction(parent)),
    deleteFile(new QAction(parent)),
    openFileTab(new QAction(parent)),
    lockUiForMasking(new QAction(parent)),
    unlockUi(new QAction(parent)) {

    // Do nothing

}