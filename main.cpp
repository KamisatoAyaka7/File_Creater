#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mainWindow;
    mainWindow.show();
    if(argc!=1) mainWindow.startOpen(argv[1]);
    else mainWindow.startNew();
    return app.exec();
}
