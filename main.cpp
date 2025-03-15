#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mainWindow;
    QFileInfo QFI1(argv[0]);
    mainWindow.absolutePath=QFI1.absolutePath();
    mainWindow.show();
    if(argc!=1)
    {
        QString filename = argv[1];
        mainWindow.startOpen(true,filename);
    }
    return app.exec();
}
