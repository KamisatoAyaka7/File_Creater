#include "mainwindow.h"
#include "codeeditor.h"
#include "hexviewer.h"
#include "configreader.h"
#include "settingsdialog.h"
#include "replacedialog.h"
#include "finddialog.h" // 新增：包含查找对话框
#include "syntaxhighlighter.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDialog>
#include <QColorDialog>
#include <QToolButton>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), settings(":/settings.ini", QSettings::IniFormat)
{
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true); // 启用标签页关闭按钮
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateStatusBar);
    setCentralWidget(tabWidget);

    createIntegratedMenuToolBar();
    createStatusBar();

    loadSettings(); // 加载设置

    saveShortcut = new QShortcut(QKeySequence::Save, this);
    openShortcut = new QShortcut(QKeySequence::Open, this);
    newShortcut = new QShortcut(QKeySequence::New, this);
    saveasShortcut = new QShortcut(QKeySequence::SaveAs, this);
    findShortcut = new QShortcut(QKeySequence::Find, this);
    replaceShortcut = new QShortcut(QKeySequence::Replace, this);
    fileLeftShortcut = new QShortcut(QKeySequence("F1"),this);
    fileRightShortcut = new QShortcut(QKeySequence("F2"),this);

    connect(saveShortcut,&QShortcut::activated,this,&MainWindow::saveFile);
    connect(openShortcut,&QShortcut::activated,this,[=](){
        openFile();
    });
    connect(newShortcut,&QShortcut::activated,this,&MainWindow::newFile);
    connect(saveasShortcut,&QShortcut::activated,this,&MainWindow::saveAs);
    connect(findShortcut,&QShortcut::activated,this,&MainWindow::showFindDialog);
    connect(replaceShortcut,&QShortcut::activated,this,&MainWindow::showReplaceDialog);
    connect(fileLeftShortcut,&QShortcut::activated,this,[=](){
        if(0<tabWidget->currentIndex())
        {
            tabWidget->setCurrentIndex(tabWidget->currentIndex()-1);
        }
    });
    connect(fileRightShortcut,&QShortcut::activated,this,[=](){
        if(tabWidget->count()-1>tabWidget->currentIndex())
        {
            tabWidget->setCurrentIndex(tabWidget->currentIndex()+1);
        }
    });

    setWindowTitle(tr("Apple_Cat"));
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    saveSettings(); // 保存设置
}

void MainWindow::newFile()
{
    CodeEditor *editor = new CodeEditor;
    connect(editor,&CodeEditor::cursorPositionChanged,this,&MainWindow::updateStatusBar);

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

    tabWidget->addTab(editor, tr("Untitled"));
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    updateStatusBar(); // 更新状态栏
}

void MainWindow::openFile(bool isStart,QString name)
{
    QString fileName="";
    if(isStart)
    {
        fileName=name;
    }
    else
    {
        fileName=QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
        if (fileName.isEmpty())
            return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file"));
        return;
    }

    QTextStream in(&file);
    CodeEditor *editor = new CodeEditor;
    connect(editor,&CodeEditor::cursorPositionChanged,this,&MainWindow::updateStatusBar);
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
    editor->setFileName(fileName);

    // 设置标签页标题和 ToolTip
    QFileInfo fileInfo(fileName);
    tabWidget->addTab(editor, fileInfo.fileName());
    tabWidget->setTabToolTip(tabWidget->count() - 1, fileInfo.absoluteFilePath());
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    updateStatusBar(); // 更新状态栏
}

void MainWindow::startOpen(bool isStart,QString name)
{
    MainWindow::openFile(isStart,name);
}

void MainWindow::saveFile()
{
    int index = tabWidget->currentIndex();
    if (index == -1)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(index));
    if (!editor)
        return;

    QString fileName = editor->toFileName();
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

    showMessage(tr("File saved successfully.")); // 显示状态消息
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

    // 设置标签页标题和 ToolTip
    QFileInfo fileInfo(fileName);
    tabWidget->addTab(editor, fileInfo.fileName());
    tabWidget->setTabToolTip(tabWidget->count() - 1, fileInfo.absoluteFilePath());
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    editor->setFileName(fileName);

    showMessage(tr("File saved successfully.")); // 显示状态消息
}

void MainWindow::closeTab(int index)
{
    tabWidget->removeTab(index);
    updateStatusBar(); // 更新状态栏
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
    SettingsDialog dialog(this);
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

void MainWindow::showReplaceDialog()
{
    ReplaceDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString findText = dialog.getFindText();
        QString replacetext = dialog.getReplaceText();
        replaceText(findText, replacetext);
    }
}

void MainWindow::replaceText(const QString &findText, const QString &replaceText)
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->currentWidget());
    if (!editor) {
        QMessageBox::warning(this, tr("Error"), tr("No active editor to replace text."));
        return;
    }

    QString text = editor->toPlainText();
    text.replace(findText, replaceText);
    editor->setPlainText(text);

    showMessage(tr("Text replaced successfully.")); // 显示状态消息
}

void MainWindow::showFindDialog()
{
    FindDialog dialog(this);
    connect(&dialog, &FindDialog::findNext, this, [this, &dialog]() {
        QString text = dialog.getFindText();
        findText(text);
    });
    dialog.exec();
}

void MainWindow::findText(const QString &text)
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->currentWidget());
    if (!editor) {
        QMessageBox::warning(this, tr("Error"), tr("No active editor to find text."));
        return;
    }

    if (editor->find(text)) {
        showMessage(tr("Text found."));
    } else {
        showMessage(tr("Text not found."));
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
    connect(openAction, &QAction::triggered, this, [=](){
        openFile();
    });

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

    QMenu *editMenu = new QMenu(tr("&Edit"), this);
    QAction *findAction = editMenu->addAction(tr("&Find..."));
    connect(findAction, &QAction::triggered, this, &MainWindow::showFindDialog);

    QAction *replaceAction = editMenu->addAction(tr("&Replace..."));
    connect(replaceAction, &QAction::triggered, this, &MainWindow::showReplaceDialog);

    QToolButton *editButton = new QToolButton(this);
    editButton->setMenu(editMenu);
    editButton->setPopupMode(QToolButton::InstantPopup);
    editButton->setText(tr("Edit"));
    menuToolBar->addWidget(editButton);

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

    QMenu *helpMenu = new QMenu(tr("&Help"), this);
    QAction *helpAction = helpMenu->addAction(tr("&Help"));
    connect(helpAction, &QAction::triggered, this, []() {
        QMessageBox::information(nullptr, tr("Help"),
                                 tr("Code Editor\n\n"
                                    "1. File: Create, open, save, and save as files.\n"
                                    "2. Edit: Find and replace text.\n"
                                    "3. View: Open Hex Viewer and change settings.\n"
                                    "4. Help: Display this help message.\n\n"
                                    "Official site: github.com/KamisatoAyaka7/File_Creater\n"
                                    "Developed by: szy\n"
                                    "Contact: yybcyzd@outlook.com"));
    });

    QToolButton *helpButton = new QToolButton(this);
    helpButton->setMenu(helpMenu);
    helpButton->setPopupMode(QToolButton::InstantPopup);
    helpButton->setText(tr("Help"));
    menuToolBar->addWidget(helpButton);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
    statusBar()->setSizeGripEnabled(false);
}

void MainWindow::updateStatusBar()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->currentWidget());
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int column = cursor.columnNumber() + 1;
        statusBar()->showMessage(tr("Line: %1, Column: %2")
                                     .arg(line)
                                     .arg(column));
    }
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

void MainWindow::showMessage(const QString &message)
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->currentWidget());
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int column = cursor.columnNumber() + 1;
        statusBar()->showMessage(tr("Line: %1, Column: %2 | ")
                                     .arg(line)
                                     .arg(column)+message);
    }
}
