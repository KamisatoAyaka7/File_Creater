#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QFileDialog>
#include <QSettings>
#include <QShortcut>
#include <QComboBox>
#include <QStringConverter>
#include "codeeditor.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void startOpen(QString name);
    void startNew();

private slots:
    void newFile();
    void saveFile();
    void openFile(QString name="");
    void saveAs();
    void closeTab(int index);
    void showHexView();
    void showSettingsDialog();
    void applySettings(const QFont &font, const QColor &backgroundColor);
    void showReplaceDialog();
    void replaceText(const QString &findText, const QString &replaceText);
    void showFindDialog();
    void findText(const QString &text);
    void updateStatusBar();
    void showMessage(const QString &message);

private:
    void createIntegratedMenuToolBar();
    void createStatusBar();
    void loadSettings();
    void bindShortCuts();
    void setCompleter(CodeEditor *editor);
    void applyCurrentSettings(CodeEditor *editor);
    void showHelpMessage();
    QString byteToString(QByteArray byte1);
    QByteArray stringToByteArray(QString str1);
    void initcodeBox();
    void reread();
    QStringConverter::Encoding toCode();

    QTabWidget *tabWidget;
    QSettings settings;
    QComboBox codeBox;
    QShortcut *saveShortcut;
    QShortcut *openShortcut;
    QShortcut *newShortcut;
    QShortcut *saveasShortcut;
    QShortcut *findShortcut;
    QShortcut *replaceShortcut;
    QShortcut *fileLeftShortcut;
    QShortcut *fileRightShortcut;
    QShortcut *reReadShortCut;
};

#endif // MAINWINDOW_H
