#include "ui.h"

#include <vector>
#include <iostream>
#include <filesystem>

#include <QFileDialog>
#include <QProgressDialog>
#include <QThreadPool>
#include <QCoreApplication>

#include "qcustomplot.h"

#include "tpx3/tpx3.h"
#include "fileview/fileview.h"

using namespace spec_hom;

enum PermanentTab : int {
    TAB_FILE_SETTINGS = 0,
    TAB_FILE_IMPORT = 1,
    TABS_NUM_PERMANENT = 2
};

MainWindow::MainWindow() :
        QMainWindow(),
        mActions(this),
        mCentralSplitter(new QSplitter(this)),
        mLogPanel(new LogPanel(mCentralSplitter)),
        mTabContainer(new QTabWidget(mCentralSplitter)),
        mFileSettingsPanel(new FileInputSettingsPanel(mTabContainer, mActions)),
        mFilePanel(new FileInputPanel(mTabContainer, *mLogPanel, mActions)),
        mActiveImportThreads(),
        mOpenImages(),
        mOpenFileViewTabs(),
        mProcessStartTime() {

    setWindowTitle("Spectral HOM Analysis");

    // Setup for global application settings
    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    setCentralWidget(mCentralSplitter);

    mCentralSplitter->addWidget(mLogPanel);
    mCentralSplitter->addWidget(mTabContainer);

    mTabContainer->setTabsClosable(true);

    mTabContainer->insertTab(TAB_FILE_SETTINGS, mFileSettingsPanel, "Import Settings");
    mTabContainer->tabBar()->setTabButton(TAB_FILE_SETTINGS, QTabBar::RightSide, nullptr);
    mTabContainer->tabBar()->setTabIcon(TAB_FILE_SETTINGS, this->style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    mTabContainer->insertTab(TAB_FILE_IMPORT,mFilePanel, "Import/Export Files");
    mTabContainer->tabBar()->setTabButton(TAB_FILE_IMPORT, QTabBar::RightSide, nullptr);
    mTabContainer->tabBar()->setTabIcon(TAB_FILE_IMPORT, this->style()->standardIcon(QStyle::SP_DialogOkButton));

    connect(mTabContainer, &QTabWidget::tabCloseRequested, this, &MainWindow::closeFileTab);

    // setup application-wide actions
    connect(mActions.close, &QAction::triggered, this, &MainWindow::closeWindow);
    connect(mActions.openFilesDialog, &QAction::triggered, mFilePanel, &FileInputPanel::openFileDialog);
    connect(mActions.startImportFiles, &QAction::triggered, this, &MainWindow::startImportFiles);
    connect(mActions.stopImportFiles, &QAction::triggered, this, &MainWindow::stopImportFiles);
    connect(mActions.clearFileList, &QAction::triggered, this, &MainWindow::deleteAllFiles);
    connect(mActions.exportAllData, &QAction::triggered, this, &MainWindow::exportAllData);
    connect(mActions.lockUiForImporting, &QAction::triggered, this, &MainWindow::freezeUiForImporting);
    connect(mActions.doneImporting, &QAction::triggered, this, &MainWindow::doneImportFiles);
    connect(mActions.deleteFile, &QAction::triggered, this, &MainWindow::deleteFiles);
    connect(mActions.openFileTab, &QAction::triggered, this, &MainWindow::openNewFileTab);
    connect(mActions.lockUiForMasking, &QAction::triggered, this, &MainWindow::freezeUiForMasking);
    connect(mActions.unlockUi, &QAction::triggered, this, &MainWindow::unfreezeUi);

    mTabContainer->setCurrentIndex(TAB_FILE_SETTINGS); // for some reason, we need this on Windows or else the tab isn't shown
    show();
    mTabContainer->setCurrentIndex(TAB_FILE_IMPORT);
    show();

}

MainWindow::~MainWindow() {

    // Do nothing

}

void MainWindow::closeEvent(QCloseEvent *event) {

    stopImportFiles();

    QMainWindow::closeEvent(event);

}

bool MainWindow::closeWindow() {

    stopImportFiles();
    while(!mActiveImportThreads.empty()); // wait until all threads closed

    return close();

}

void MainWindow::freezeUiForImporting() {

    mTabContainer->setCurrentWidget(mFilePanel); // activate Import Files tab
    mTabContainer->setTabEnabled(TAB_FILE_SETTINGS, false); // disable Import Settings tab
    for(int ix = TABS_NUM_PERMANENT; ix < mTabContainer->count(); ++ix) // disable any viewing tabs
        mTabContainer->setTabEnabled(ix, false);

    mFilePanel->setCancelBtnOnly(true);

}

void MainWindow::freezeUiForMasking() {

    mTabContainer->setTabEnabled(TAB_FILE_IMPORT, false); // disable Import Files tab
    mTabContainer->setCurrentWidget(mFileSettingsPanel); // activate Import Settings tab
    for(int ix = TABS_NUM_PERMANENT; ix < mTabContainer->count(); ++ix) // disable any viewing tabs
        mTabContainer->setTabEnabled(ix, false);

    mFileSettingsPanel->setEnabled(false);

}

void MainWindow::unfreezeUi() {

    setEnabled(true);
    mFilePanel->setCancelBtnOnly(false);
    mFileSettingsPanel->setEnabled(true);

    for(auto ix = 0; ix < mTabContainer->count(); ++ix)
        mTabContainer->setTabEnabled(ix, true);

}

void MainWindow::exportAllData() {

    auto folder = QFileDialog::getExistingDirectory(
            this,
            "Select Export Folder",
            "."
    );

    if(folder.isEmpty())
        return;

    mLogPanel->log("Exporting to " + folder.toStdString() + "...");

    for(auto &pair : mOpenImages) {
        auto &filepath = pair.first;
        auto &image = *pair.second;

        std::filesystem::path p(filepath);
        std::string output_path = folder.toStdString() + "/" + p.stem().string() + ".pairs.csv";
        image.saveTo(output_path);
        mLogPanel->log("Wrote file " + output_path);
    }

    mLogPanel->log("Done exporting");

}

void MainWindow::deleteFiles() {

    QVariant data = mActions.deleteFile->data();
    mActions.deleteFile->setData({});

    if(data.isNull()) {
        throw std::runtime_error("Call to MainWindow::deleteFile(), but no filename provided.");
    }

    QStringList files_to_delete = data.toStringList();

    if(files_to_delete.isEmpty()) {
        throw std::runtime_error("Call to MainWindow::deleteFile(), but filename is not a QString");
    }

    unsigned files_deleted = 0;

    for(auto &q_str : files_to_delete) {
        auto str = q_str.toStdString();
        // close the viewing tab too, if necessary
        if(mOpenFileViewTabs.contains(str)) {
            auto tab = mOpenFileViewTabs[str];
            auto tab_ix = mTabContainer->indexOf(tab);
            closeFileTab(tab_ix);
        }

        files_deleted += mOpenImages.erase(str);
    }

    if(files_deleted)
        mLogPanel->log(std::to_string(files_deleted) + " files deleted.");

}

void MainWindow::deleteAllFiles() {

    unsigned files_deleted = mOpenImages.size();

    // remove all open display tabs
    while(!mOpenFileViewTabs.empty()) {
        auto &tab_pair = *mOpenFileViewTabs.begin();
        auto tab_ix = mTabContainer->indexOf(tab_pair.second);
        closeFileTab(tab_ix);
    }

    mOpenImages.clear();
    mFilePanel->clearAllRows();

    if(files_deleted)
        mLogPanel->log(std::to_string(files_deleted) + " files deleted.");

}

void MainWindow::startImportFiles() {

    auto queued_files = mFilePanel->queuedFileList();

    freezeUiForImporting();

    auto import_settings = mFileSettingsPanel->getSettings();

    mLogPanel->log("Loading " + std::to_string(queued_files.size()) + " Tpx3 files.");

    QThreadPool::globalInstance()->setMaxThreadCount(import_settings.maxNumThreads);
    mProcessStartTime = std::chrono::high_resolution_clock::now();

    for(auto &file : queued_files) {
        auto file_loader = new LoadRawFileThread(file, import_settings);
        mLogPanel->connectToThread(file_loader);
        mFilePanel->connectThread(file, file_loader);

        connect(file_loader, &LoadRawFileThread::yieldPixelData, this, &MainWindow::receiveImageData);

        QThreadPool::globalInstance()->start(file_loader);

        // keep track of which threads are currently running
        mActiveImportThreads.push_back(file_loader);
        connect(file_loader, &LoadRawFileThread::threadDone, this, [this, file_loader]() {
            this->mActiveImportThreads.erase(
                    std::remove(this->mActiveImportThreads.begin(), this->mActiveImportThreads.end(), file_loader),
                    this->mActiveImportThreads.end());
        });
    }

}

void MainWindow::stopImportFiles() {

    for(auto thread : mActiveImportThreads)
        thread->cancel();

}

void MainWindow::doneImportFiles() {

    auto stop_time = std::chrono::high_resolution_clock::now();
    mLogPanel->log("File loading took " + std::to_string((stop_time - mProcessStartTime).count() / 1e9) + " seconds.");

    unfreezeUi();

}

void MainWindow::receiveImageData(Tpx3Image *data) {

    std::unique_ptr<Tpx3Image> image(data);
    auto &filename = image->fullFilename();

    // empty data sent if the user pressed "Cancel"
    if(!image->empty()) {
        mOpenImages[filename] = std::move(image);
        mFilePanel->setFileLoaded(filename);
    } else {
        mFilePanel->setFileQueued(filename);
    }

}

void MainWindow::openNewFileTab() {

    QVariant data = mActions.openFileTab->data();
    mActions.openFileTab->setData({});

    if(data.isNull()) {
        throw std::runtime_error("Call to MainWindow::openNewFileTab(), but no filename provided.");
    }

    QString q_file_to_open = data.toString();

    if(q_file_to_open.isEmpty()) {
        throw std::runtime_error("Call to MainWindow::openNewFileTab(), but filename is not a QString");
    }

    auto file_to_open = q_file_to_open.toStdString();

    if(!mOpenImages.contains(file_to_open)) {
        mLogPanel->err("Failed to lookup file " + file_to_open);
    }

    if(mOpenFileViewTabs.contains(file_to_open)) {
        mTabContainer->setCurrentWidget(mOpenFileViewTabs[file_to_open]);
        return;
    }

    // add a new tab to view the file
    Tpx3Image *image = mOpenImages[file_to_open].get();
    FileViewer *view_tab = new FileViewer(mTabContainer, image);
    auto filename = image->filename();

    mOpenFileViewTabs[file_to_open] = view_tab;
    mTabContainer->addTab(view_tab, filename.c_str());
    mTabContainer->setCurrentWidget(view_tab);

}

void MainWindow::closeFileTab(int index) {

    auto tab = dynamic_cast<FileViewer*>(mTabContainer->widget(index));
    assert(tab); // check for nullptr

    auto filename = tab->fullFilename();
    mOpenFileViewTabs.erase(filename); // remove from list of open tabs

    mTabContainer->removeTab(index);

}