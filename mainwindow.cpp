#include "mainwindow.h"
#include "codeeditor.h"
#include "hexviewer.h"
#include "configreader.h"
#include "settingsdialog.h"
#include "replacedialog.h"
#include "finddialog.h"
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
#include <QComboBox>
#include <QStringConverter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), settings(":/settings.ini", QSettings::IniFormat)
{
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true); // 启用标签页关闭按钮
    tabWidget->setMovable(true);
    //tabWidget->setTabShape(QTabWidget::Triangular);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateStatusBar);
    setCentralWidget(tabWidget);

    initcodeBox();

    createIntegratedMenuToolBar();
    createStatusBar();

    loadSettings(); // 加载设置
    bindShortCuts();

    setWindowTitle(tr("Apple_Cat"));
    resize(800, 600);
}

void MainWindow::bindShortCuts()
{
    saveShortcut = new QShortcut(QKeySequence::Save, this);
    openShortcut = new QShortcut(QKeySequence::Open, this);
    newShortcut = new QShortcut(QKeySequence::New, this);
    saveasShortcut = new QShortcut(QKeySequence::SaveAs, this);
    findShortcut = new QShortcut(QKeySequence::Find, this);
    replaceShortcut = new QShortcut(QKeySequence::Replace, this);
    fileLeftShortcut = new QShortcut(QKeySequence("F1"),this);
    fileRightShortcut = new QShortcut(QKeySequence("F2"),this);
    reReadShortCut = new QShortcut(QKeySequence("Ctrl+r"),this);

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
    connect(reReadShortCut,&QShortcut::activated,this,&MainWindow::reread);
}

void MainWindow::initcodeBox()
{
    QStringList codeList={"Utf-8","Utf-16","Utf-16BE","Utf-16LE",
        "Utf-32","Utf-32BE","Utf-32LE","Latin1","System"};
    codeBox.addItems(codeList);
    codeBox.setEditable(false);
    codeBox.setCurrentText("UTF-8");
}

void MainWindow::setCompleter(CodeEditor *editor)
{
    // 设置高亮器和补全器
    QFileInfo fileinfo(editor->toFileName());
    QString fileSuffix=fileinfo.suffix();
    ConfigReader *configReader;

    {
        if(fileSuffix=="h") configReader=new ConfigReader(":/cpp_syntax_config.json");
        else if(fileSuffix=="cpp") configReader=new ConfigReader(":/cpp_syntax_config.json");
        else if(fileSuffix=="py") configReader=new ConfigReader(":/python_syntax_config.json");
        else if(fileSuffix=="pyw") configReader=new ConfigReader(":/python_syntax_config.json");
        else if(fileSuffix=="js") configReader=new ConfigReader(":/javascript_syntax_config.json");
        else if(fileSuffix=="java") configReader=new ConfigReader(":/java_syntax_config.json");
        else if(fileSuffix=="cs") configReader=new ConfigReader(":/csharp_syntax_config.json");
        else if(fileSuffix=="rs") configReader=new ConfigReader(":/rust_syntax_config.json");
        else if(fileSuffix=="rb") configReader=new ConfigReader(":/ruby_syntax_config.json");
        else configReader=new ConfigReader(":/default_syntax_config.json");
    }

    QMap<QString, QColor> keywords = configReader->getKeywords();
    editor->setCompleter(new QCompleter(keywords.keys(), editor));
    new SyntaxHighlighter(editor->document(), keywords);
}

void MainWindow::applyCurrentSettings(CodeEditor *editor)
{
    // 应用当前设置
    QFont font = settings.value("font", QFont("Monospace", 12)).value<QFont>();
    QColor backgroundColor = settings.value("backgroundColor", QColor(Qt::white)).value<QColor>();
    editor->setFont(font);
    QPalette palette = editor->palette();
    palette.setColor(QPalette::Base, backgroundColor);
    editor->setPalette(palette);
}

void MainWindow::createIntegratedMenuToolBar()
{
    QToolBar *menuToolBar = addToolBar(tr("Menu"));
    menuToolBar->setMovable(false);

    {
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
    }

    {
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
    }

    {
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

    {
        QMenu *helpMenu = new QMenu(tr("&Help"), this);
        QAction *helpAction = helpMenu->addAction(tr("&Help"));
        connect(helpAction, &QAction::triggered, this, &MainWindow::showHelpMessage);

        QToolButton *helpButton = new QToolButton(this);
        helpButton->setMenu(helpMenu);
        helpButton->setPopupMode(QToolButton::InstantPopup);
        helpButton->setText(tr("Help"));
        menuToolBar->addWidget(helpButton);
    }
    menuToolBar->addWidget(&codeBox);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
    statusBar()->setSizeGripEnabled(false);
}

QStringConverter::Encoding MainWindow::toCode()
{
    QStringConverter::Encoding list[]={
        QStringConverter::Utf8,
        QStringConverter::Utf16,
        QStringConverter::Utf16BE,
        QStringConverter::Utf16LE,
        QStringConverter::Utf32,
        QStringConverter::Utf32BE,
        QStringConverter::Utf32LE,
        QStringConverter::Latin1,
        QStringConverter::System};
    return list[codeBox.currentIndex()];
}

/*===========================================*/

void MainWindow::newFile()
{
    CodeEditor *editor = new CodeEditor;
    connect(editor,&CodeEditor::cursorPositionChanged,this,&MainWindow::updateStatusBar);

    applyCurrentSettings(editor);

    tabWidget->addTab(editor, tr("Untitled"));
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    updateStatusBar(); // 更新状态栏
}

void MainWindow::startNew()
{
    newFile();
    tabWidget->currentWidget()->setFocus();
}

QString MainWindow::byteToString(QByteArray byte1)
{
    QStringDecoder decoder(toCode());
    return decoder.decode(byte1);
}

/*===========================================*/

void MainWindow::openFile(QString name)
{
    QString fileName="";
    if(name!="")
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

    CodeEditor *editor = new CodeEditor;
    connect(editor,&CodeEditor::cursorPositionChanged,this,&MainWindow::updateStatusBar);

    applyCurrentSettings(editor);
    editor->setFileName(fileName);
    setCompleter(editor);

    // 设置标签页标题和 ToolTip
    QFileInfo fileInfo(fileName);
    tabWidget->addTab(editor, fileInfo.fileName());
    tabWidget->setTabToolTip(tabWidget->count() - 1, fileInfo.absoluteFilePath());
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    editor->setPlainText(byteToString(file.readAll()));
    file.close();

    updateStatusBar(); // 更新状态栏
}

void MainWindow::reread()
{
    int index = tabWidget->currentIndex();

    if (index == -1)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(index));
    if (!editor)
        return;

    QString fileName = editor->toFileName();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file"));
        return;
    }
    editor->setPlainText(byteToString(file.readAll()));
    file.close();

    updateStatusBar(); // 更新状态栏
}

void MainWindow::startOpen(QString name)
{
    MainWindow::openFile(name);
}

/*===========================================*/

QByteArray MainWindow::stringToByteArray(QString str1)
{
    QStringEncoder encoder(toCode());
    return encoder.encode(str1);
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

    file.write(stringToByteArray(editor->toPlainText()));
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

    file.write(stringToByteArray(editor->toPlainText()));
    file.close();

    tabWidget->setTabText(index, fileName);

    // 设置标签页标题和 ToolTip
    QFileInfo fileInfo(fileName);
    tabWidget->addTab(editor, fileInfo.fileName());
    tabWidget->setTabToolTip(tabWidget->count() - 1, fileInfo.absoluteFilePath());
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    editor->setFileName(fileName);
    setCompleter(editor);

    showMessage(tr("File saved successfully.")); // 显示状态消息
}

/*===========================================*/

void MainWindow::closeTab(int index)
{
    tabWidget->removeTab(index);
    updateStatusBar(); // 更新状态栏
}

/*===========================================*/

void MainWindow::showHexView()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
    if (fileName.isEmpty())
        return;

    HexViewer *hexViewer = new HexViewer;
    hexViewer->loadFile(fileName);
    hexViewer->show();
}

/*===========================================*/

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

void MainWindow::showHelpMessage()
{
    QFile file(":/apple_cat.txt");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    CodeEditor *editor = new CodeEditor;
    editor->setPlainText(in.readAll());
    editor->setReadOnly(true);
    file.close();

    applyCurrentSettings(editor);

    tabWidget->addTab(editor, "Help");
    tabWidget->setCurrentIndex(tabWidget->count()-1);

    updateStatusBar(); // 更新状态栏
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

/*===========================================*/


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

/*===========================================*/

MainWindow::~MainWindow()
{

}
