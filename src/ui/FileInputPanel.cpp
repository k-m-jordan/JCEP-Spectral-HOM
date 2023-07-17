#include "ui.h"

#include <algorithm>
#include <iostream>
#include <fstream>

#include <QHeaderView>
#include <QFileDialog>

#include "tpx3/tpx3.h"

using namespace spec_hom;

// Column indices
enum TableColumn : int {
    COL_DELETE_BTN = 0,
    COL_FILE_NAME,
    COL_FILE_SIZE,
    COL_PROG_BAR,
    COL_NUM
};

FileInputPanel::FileInputPanel(QWidget *parent, LogPanel &logger, AppActions &actions) :
        QWidget(parent),
        mLogger(logger),
        mActions(actions),
        mLayout(new QVBoxLayout(this)),
        mToolbar(new QWidget(this)),
        mToolbarLayout(new QHBoxLayout(mToolbar)),
        mOpenFileBtn(new QPushButton(this)),
        mStartStopLoadBtn(new QPushButton(this)),
        mClearFilesBtn(new QPushButton(this)),
        mFileTable(new QTableWidget(this)),
        mBottomText(new QLabel(this)),
        mCancelBtnOnly(false),
        mFileList() {

    setLayout(mLayout);

    // Button to open Tpx3 files
    mOpenFileBtn->setText("Add Tpx3 Files");
    mOpenFileBtn->setIcon(this->style()->standardIcon(QStyle::SP_DialogOpenButton));
    mOpenFileBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    //connect(mOpenFileBtn, &QPushButton::clicked, actions.openFileDialog, &QAction::trigger); // TODO: remove; old file handler
    connect(mOpenFileBtn, &QPushButton::clicked, actions.openFilesDialog, &QAction::trigger);

    // Button to load Tpx3 files
    mStartStopLoadBtn->setText("Import All");
    mStartStopLoadBtn->setEnabled(false);
    mStartStopLoadBtn->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    mStartStopLoadBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(mStartStopLoadBtn, &QPushButton::clicked, this, &FileInputPanel::startStopBtnClick);

    // Button to clear file list
    mClearFilesBtn->setText("Clear List");
    mClearFilesBtn->setIcon(this->style()->standardIcon(QStyle::SP_DialogResetButton));
    mClearFilesBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(mClearFilesBtn, &QPushButton::clicked, actions.clearFileList, &QAction::trigger);

    // Table displaying open files
    mFileTable->setColumnCount(COL_NUM);
    mFileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mFileTable->horizontalHeader()->setStretchLastSection(true);
    mFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mFileTable->setFocusPolicy(Qt::NoFocus);
    mFileTable->setSelectionMode(QAbstractItemView::NoSelection);
    mFileTable->setHorizontalHeaderLabels({"", "Path", "Size (MB)", "Status"});

    connect(mFileTable, &QTableWidget::cellDoubleClicked, this, &FileInputPanel::tableDoubleClick);

    // Setup toolbar layout
    mToolbar->setLayout(mToolbarLayout);
    mToolbarLayout->addWidget(mOpenFileBtn);
    mToolbarLayout->addWidget(mStartStopLoadBtn);
    mToolbarLayout->addWidget(mClearFilesBtn);

    // Setup bottom descriptive text
    mBottomText->setText("Double click a file to view data.");

    // Setup panel layout
    mLayout->addWidget(mToolbar);
    mLayout->addWidget(mFileTable);
    mLayout->addWidget(mBottomText);

}

void FileInputPanel::openFileDialog() {

    auto filelist = QFileDialog::getOpenFileNames(
            this,
            "Open Raw Tpx3 Files",
            "./data",
            "Tpx3 Files (*.tpx3)"
    );

    if(filelist.isEmpty())
        return;

    // STL list of paths
    std::vector<std::string> paths;
    for(auto &file : filelist)
        paths.push_back(file.toStdString());

    addQueuedFiles(paths);

}

void FileInputPanel::addQueuedFiles(std::vector<std::string> paths) {

    // if any files are already in mFileList, we don't need to import them
    paths.erase(std::remove_if(paths.begin(), paths.end(), [this](const auto&x) {
        return std::find(this->mFileList.begin(), this->mFileList.end(), x) != this->mFileList.end();
    }), paths.end());

    // append to list
    mFileList.insert(mFileList.end(), paths.begin(), paths.end());

    // sort mFileList and remove duplicates
    std::sort(mFileList.begin(), mFileList.end());
    mFileList.erase(std::unique(mFileList.begin(), mFileList.end()), mFileList.end());

    updateFileTable();

}

void FileInputPanel::updateFileTable() {

    // make a list of files already loaded, so that we don't reset their progress bars
    std::vector<std::string> loaded_files;
    for(unsigned row = 0; row < mFileTable->rowCount(); ++row) {
        auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(row, COL_PROG_BAR));
        if(pbar->loaded()) {
            auto filename = mFileTable->item(row, COL_FILE_NAME)->text();
            loaded_files.push_back(filename.toStdString());
        }
    }

    mFileTable->setRowCount(mFileList.size());

    for(auto ix = 0; ix < mFileList.size(); ++ix) {
        auto &filename = mFileList[ix];

        std::ifstream fs(filename, std::ifstream::ate | std::ifstream::binary);
        auto file_size = static_cast<double>(fs.tellg()) / (1024*1024);
        auto file_size_str = std::to_string(file_size);
        // format the string
        file_size_str.erase(file_size_str.find('.', 0)+3);

        auto close_btn = new QPushButton(this->style()->standardIcon(QStyle::SP_DialogCloseButton), "");
        connect(close_btn, &QPushButton::clicked, this, [ix, this]() {
            this->removeTableRow(ix);
        });

        auto filename_cell = new QTableWidgetItem(filename.c_str());

        auto file_size_cell = new QTableWidgetItem(file_size_str.c_str());
        file_size_cell->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto file_status_cell = new FileImportProgressBar(this);

        if(std::find(loaded_files.begin(), loaded_files.end(), filename) != loaded_files.end()) {
            file_status_cell->setLoaded();
        } else {
            file_status_cell->setQueued();
        }

        mFileTable->setCellWidget(ix, COL_DELETE_BTN, close_btn);
        mFileTable->setItem(ix, COL_FILE_NAME, filename_cell);
        mFileTable->setItem(ix, COL_FILE_SIZE, file_size_cell);
        mFileTable->setCellWidget(ix, COL_PROG_BAR, file_status_cell);
    }

    mStartStopLoadBtn->setEnabled(!queuedFileList().empty());

}

void FileInputPanel::removeTableRow(int row) {

    // check if this file is loaded; if so, we need to delete it from memory
    auto row_file_name = mFileTable->item(row, COL_FILE_NAME)->text();
    mActions.deleteFile->setData(row_file_name);
    mActions.deleteFile->trigger();

    // the obvious solution using removeRow() leads to segfaults, so do a manual row deletion
    int num_rows = mFileTable->rowCount();
    for(int r = row; r < (num_rows-1); ++r) {
        mFileTable->setItem(r, COL_FILE_NAME, mFileTable->item(r + 1, COL_FILE_NAME)->clone());
        mFileTable->setItem(r, COL_FILE_SIZE, mFileTable->item(r + 1, COL_FILE_SIZE)->clone());

        auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(r + 1, COL_PROG_BAR));
        auto new_pbar = pbar->clone(this);
        new_pbar->connectThread(pbar);
        mFileTable->setCellWidget(r, COL_PROG_BAR, new_pbar);
    }
    mFileTable->setRowCount(num_rows - 1);
    mFileList.erase(mFileList.begin() + row);

    if(queuedFileList().empty())
        mStartStopLoadBtn->setEnabled(false);

}

void FileInputPanel::clearAllRows() {

    mFileList.clear();
    mFileTable->setRowCount(0);
    mStartStopLoadBtn->setEnabled(false);

}

void FileInputPanel::setCancelBtnOnly(bool value) {

    mOpenFileBtn->setEnabled(!value);
    mClearFilesBtn->setEnabled(!value);

    unsigned num_rows = mFileTable->rowCount();
    for(int r = 0; r < num_rows; ++r) {
        mFileTable->cellWidget(r, COL_DELETE_BTN)->setEnabled(!value);
    }

    if(value) {
        mStartStopLoadBtn->setText("Cancel");
        mStartStopLoadBtn->setIcon(this->style()->standardIcon(QStyle::SP_DialogCancelButton));
        mStartStopLoadBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    } else {
        mStartStopLoadBtn->setText("Import All");
        mStartStopLoadBtn->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
        mStartStopLoadBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    }

    mStartStopLoadBtn->setEnabled(!queuedFileList().empty());

    mCancelBtnOnly = value;

}

std::vector<std::string> FileInputPanel::queuedFileList() const {

    std::vector<std::string> queued_files;

    for(auto row = 0; row < mFileList.size(); ++row) {

        auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(row, COL_PROG_BAR));
        if(pbar->queued())
            queued_files.push_back(mFileList[row]);

    }

    return queued_files;

}

void FileInputPanel::startStopBtnClick() {

    if(mCancelBtnOnly) {
        mActions.stopImportFiles->trigger();
    } else {
        if(mFileList.empty())
            return;

        setCancelBtnOnly(true);
        mActions.startImportFiles->trigger();
    }

}

int FileInputPanel::getFileRow(const std::string &file) {

    QString filename_qt = QString(file.c_str());

    // iterate through rows until we find the one corresponding to file
    for(int r = 0; r < mFileTable->rowCount(); ++r) {

        auto row_txt = mFileTable->item(r, COL_FILE_NAME)->text();
        if(row_txt == filename_qt)
            return r;

    }

    return -1;

}

void FileInputPanel::connectThread(const std::string &file, LoadRawFileThread *thread) {

    int row = getFileRow(file);
    assert(row != -1); // should never happen if UI is written correctly

    auto status_widget = mFileTable->cellWidget(row, COL_PROG_BAR);
    auto pbar = dynamic_cast<FileImportProgressBar*>(status_widget);

    pbar->connectThread(thread);
    pbar->setIsLoading();

    connect(thread, &LoadRawFileThread::threadDone, this, &FileInputPanel::updateLoadStatus);

}

void FileInputPanel::setFileLoaded(const std::string &file) {

    int row = getFileRow(file);
    assert(row != -1); // should never happen if UI is written correctly

    auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(row, COL_PROG_BAR));
    pbar->setLoaded();

}

void FileInputPanel::setFileQueued(const std::string &file) {

    int row = getFileRow(file);
    assert(row != -1); // should never happen if UI is written correctly

    auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(row, COL_PROG_BAR));
    pbar->setQueued();

}

void FileInputPanel::updateLoadStatus() {

    int num_rows = mFileTable->rowCount();

    bool files_loading = false;

    for(int row = 0; row < num_rows; ++row) {
        auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(row, COL_PROG_BAR));
        files_loading |= pbar->isLoading();
    }

    if(!files_loading) {
        mActions.doneImporting->trigger();
    }

}

void FileInputPanel::tableDoubleClick(int row, int col) {

    if(mCancelBtnOnly) {
        mLogger.warn("Cannot open file for viewing while importing files.");
        return; // not accepting input at this time
    }

    assert(row < mFileTable->rowCount());
    assert(col < COL_NUM);

    // prevent accidental calls that should be a button press instead
    if(col == COL_DELETE_BTN)
        return;

    auto filename = mFileTable->item(row, COL_FILE_NAME)->text();
    auto pbar = dynamic_cast<FileImportProgressBar*>(mFileTable->cellWidget(row, COL_PROG_BAR));

    if(!pbar->loaded()) {
        mLogger.warn("Cannot display a file that has not been imported.");
        return;
    }

    mActions.openFileTab->setData(filename);
    mActions.openFileTab->trigger();

}