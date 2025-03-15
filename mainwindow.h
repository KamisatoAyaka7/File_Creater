#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QFileDialog>
#include <QTextCodec>
#include <QSettings>
#include <QShortcut>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void startOpen(QString Name);
    QString absolutePath;

    void startOpen(bool isStart,QString name);

private slots:
    void newFile();
    void saveFile();
    void openFile(bool isStart=false,QString name="");
    void saveAs();
    void closeTab(int index);
    void showHexView();
    void showSettingsDialog();
    void applySettings(const QFont &font, const QColor &backgroundColor);
    void showReplaceDialog();
    void replaceText(const QString &findText, const QString &replaceText);
    void showFindDialog(); // 新增：显示查找对话框
    void findText(const QString &text); // 新增：查找文本

    void updateStatusBar(); // 新增：更新状态栏
    void showMessage(const QString &message); // 新增：显示状态消息

private:
    void createIntegratedMenuToolBar();
    void createStatusBar();
    void loadSettings();
    void saveSettings();

    QTabWidget *tabWidget;
    QSettings settings;
    QShortcut *saveShortcut;
    QShortcut *openShortcut;
    QShortcut *newShortcut;
    QShortcut *saveasShortcut;
    QShortcut *findShortcut;
    QShortcut *replaceShortcut;
};

#endif // MAINWINDOW_H
