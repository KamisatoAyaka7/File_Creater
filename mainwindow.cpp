#include "mainwindow.h"
#include "codeeditor.h"
#include "hexviewer.h"
#include "configreader.h"
#include "settingsdialog.h"
#include "syntaxhighlighter.h"
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDialog>
#include <QColorDialog>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), settings("settings.ini", QSettings::IniFormat)
{
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true); // 启用标签页关闭按钮
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    setCentralWidget(tabWidget);

    createIntegratedMenuToolBar();

    loadSettings(); // 加载设置

    setWindowTitle(tr("File_Creater"));
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    saveSettings(); // 保存设置
}

void MainWindow::newFile()
{
    CodeEditor *editor = new CodeEditor;

    // 设置高亮器和补全器
    ConfigReader configReader("syntax_config.json");
    QMap<QString, QColor> keywords = configReader.getKeywords();
    editor->setCompleter(new QCompleter(keywords.keys(), editor));
    new SyntaxHighlighter(editor->document(), keywords);

    // 应用当前设置
    QFont font = settings.value("font", QFont("Monospace", 12)).value<QFont>();
    QColor backgroundColor = settings.value("backgroundColor", QColor(Qt::white)).value<QColor>();
    editor->setFont(font);
    QPalette palette = editor->palette();
    palette.setColor(QPalette::Base, backgroundColor);
    editor->setPalette(palette);

    tabWidget->addTab(editor, tr("Untitled"));
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file"));
        return;
    }

    QTextStream in(&file);
    CodeEditor *editor = new CodeEditor;
    editor->setPlainText(in.readAll());
    file.close();

    // 设置高亮器和补全器
    ConfigReader configReader(":/syntax_config.json");
    QMap<QString, QColor> keywords = configReader.getKeywords();
    editor->setCompleter(new QCompleter(keywords.keys(), editor));
    new SyntaxHighlighter(editor->document(), keywords);

    // 应用当前设置
    QFont font = settings.value("font", QFont("Monospace", 12)).value<QFont>();
    QColor backgroundColor = settings.value("backgroundColor", QColor(Qt::white)).value<QColor>();
    editor->setFont(font);
    QPalette palette = editor->palette();
    palette.setColor(QPalette::Base, backgroundColor);
    editor->setPalette(palette);

    tabWidget->addTab(editor, fileName);
}

void MainWindow::saveFile()
{
    int index = tabWidget->currentIndex();
    if (index == -1)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(index));
    if (!editor)
        return;

    QString fileName = tabWidget->tabText(index);
    if (fileName == tr("Untitled")) {
        saveAs();
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file"));
        return;
    }

    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();
}

void MainWindow::saveAs()
{
    int index = tabWidget->currentIndex();
    if (index == -1)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(index));
    if (!editor)
        return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", tr("All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file"));
        return;
    }

    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();
    tabWidget->setTabText(index, fileName);
}

void MainWindow::closeTab(int index)
{
    tabWidget->removeTab(index);
}

void MainWindow::showHexView()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
    if (fileName.isEmpty())
        return;

    HexViewer *hexViewer = new HexViewer;
    hexViewer->loadFile(fileName);
    hexViewer->show();
}

void MainWindow::showSettingsDialog()
{
    QFont currentFont = settings.value("font", QFont("Monospace", 12)).value<QFont>();
    QColor currentBackgroundColor = settings.value("backgroundColor", QColor(Qt::white)).value<QColor>();

    SettingsDialog dialog(currentFont, currentBackgroundColor, this);
    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);
    dialog.exec();
}

void MainWindow::applySettings(const QFont &font, const QColor &backgroundColor)
{
    // 保存设置到文件
    settings.setValue("font", font);
    settings.setValue("backgroundColor", backgroundColor);

    // 应用设置到所有打开的编辑器
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (editor) {
            editor->setFont(font);
            QPalette palette = editor->palette();
            palette.setColor(QPalette::Base, backgroundColor);
            editor->setPalette(palette);
        }
    }
}

void MainWindow::createIntegratedMenuToolBar()
{
    QToolBar *menuToolBar = addToolBar(tr("Menu"));
    menuToolBar->setMovable(false);

    QMenu *fileMenu = new QMenu(tr("&File"), this);
    QAction *newAction = fileMenu->addAction(tr("&New"));
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    QAction *saveAction = fileMenu->addAction(tr("&Save"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);

    QAction *saveAsAction = fileMenu->addAction(tr("Save &As..."));
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);

    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction(tr("&Exit"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QToolButton *fileButton = new QToolButton(this);
    fileButton->setMenu(fileMenu);
    fileButton->setPopupMode(QToolButton::InstantPopup);
    fileButton->setText(tr("File"));
    menuToolBar->addWidget(fileButton);

    QMenu *viewMenu = new QMenu(tr("&View"), this);
    QAction *hexViewAction = viewMenu->addAction(tr("&Hex Viewer"));
    connect(hexViewAction, &QAction::triggered, this, &MainWindow::showHexView);

    QAction *settingsAction = viewMenu->addAction(tr("&Settings"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);

    QToolButton *viewButton = new QToolButton(this);
    viewButton->setMenu(viewMenu);
    viewButton->setPopupMode(QToolButton::InstantPopup);
    viewButton->setText(tr("View"));
    menuToolBar->addWidget(viewButton);
}

void MainWindow::loadSettings()
{
    // 从文件加载设置
    QFont font = settings.value("font", QFont("Monospace", 12)).value<QFont>();
    QColor backgroundColor = settings.value("backgroundColor", QColor(Qt::white)).value<QColor>();

    // 应用设置到所有打开的编辑器
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (editor) {
            editor->setFont(font);
            QPalette palette = editor->palette();
            palette.setColor(QPalette::Base, backgroundColor);
            editor->setPalette(palette);
        }
    }
}

void MainWindow::saveSettings()
{
    // 设置已在 applySettings 中保存
}
