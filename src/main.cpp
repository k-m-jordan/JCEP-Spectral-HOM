#include <QApplication>
#include "ui/ui.h"

using namespace spec_hom;

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);

    MainWindow mainWindow;
    mainWindow.showMaximized();

    return QApplication::exec();

}
