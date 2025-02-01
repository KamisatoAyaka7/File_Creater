#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QFileDialog>
#include <QTextCodec>
#include <QSettings>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveAs();
    void closeTab(int index);
    void showHexView();
    void showSettingsDialog();
    void applySettings(const QFont &font, const QColor &backgroundColor);

private:
    void createIntegratedMenuToolBar();
    void loadSettings();
    void saveSettings();

    QTabWidget *tabWidget;
    QSettings settings;
};

#endif // MAINWINDOW_H
