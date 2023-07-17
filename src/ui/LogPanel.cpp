#include "log.h"

#include "threadutils.h"

using namespace spec_hom;

LogPanel::LogPanel(QWidget *parent) :
    QPlainTextEdit(parent) {

    setReadOnly(true);

}

void LogPanel::log(std::string str) {

    std::string html_str = "<b>Log:</b> " + str;
    appendHtml(html_str.c_str());

}

void LogPanel::warn(std::string str) {

    std::string html_str = "<b style=\"color:orange\">Warning:</b> " + str;
    appendHtml(html_str.c_str());

}

void LogPanel::err(std::string str) {

    std::string html_str = "<b style=\"color:red\">Error:</b> " + str;
    appendHtml(html_str.c_str());

}

void LogPanel::connectToThread(BgThread *thread) {

    // signals allowing the thread to log to the main window
    connect(thread, &BgThread::log, this, &LogPanel::log);
    connect(thread, &BgThread::warn, this, &LogPanel::warn);
    connect(thread, &BgThread::err, this, &LogPanel::err);

}