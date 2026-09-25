#include "mainwindow.h"

#include "appsettings.h"
#include "coreinit.h"
#include "docview.h"
#include "editortab.h"
#include "encodingservice.h"
#include "hexeditor.h"
#include "pluginhost.h"
#include "selftest.h"
#include "syntaxrepository.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QTimer>
#include <QToolBar>
#include <QUrl>

#include "codeeditor.h"
#include "settingsdialog.h"

namespace AppleCat::Gui {

using AppleCat::Core::AppSettings;
using AppleCat::Core::EncodingService;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowIcon(QIcon(QStringLiteral(":/qt-project.org/icons/Qtx")));
    setAcceptDrops(true);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    setCentralWidget(m_tabWidget);

    createMenus();
    createToolbar();
    createStatusBar();

    m_pluginHost = new PluginHost(this);
    m_pluginHost->setMainWindow(this);

    restoreGeometry(AppSettings::instance()->value(AppSettings::kWindowGeometry).toByteArray());
    setWindowTitle(tr("Apple_Cat"));
    resize(960, 680);

    // Load plugins once menus exist so they can contribute UI right away.
    m_pluginHost->loadAll();

    // Live settings application for every open document.
    connect(AppSettings::instance(), &AppSettings::settingsChanged, this,
            &MainWindow::applySettingsToTabs);
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus()
{
    // File --------------------------------------------------------------
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::openFile);
    m_recentMenu = fileMenu->addMenu(tr("Open &Recent"));
    connect(m_recentMenu, &QMenu::aboutToShow, this, [this] {
        m_recentMenu->clear();
        const QStringList recent =
            AppSettings::instance()->value(AppSettings::kRecentFiles).toStringList();
        if (recent.isEmpty()) {
            QAction *empty = m_recentMenu->addAction(tr("(empty)"));
            empty->setEnabled(false);
        } else {
            for (const QString &f : recent)
                m_recentMenu->addAction(f, this, [this, f] { openPath(f); });
        }
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this,
                        &MainWindow::saveFileAs);
    fileMenu->addAction(tr("&Reload"), QKeySequence(QStringLiteral("Ctrl+R")), this,
                        &MainWindow::reloadTab);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Open in &Hex Editor"), QKeySequence(QStringLiteral("Ctrl+Shift+H")),
                        this, [this] { openInHex(currentDocumentPathForPlugins()); });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);

    // Edit ----------------------------------------------------------------
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Undo"), QKeySequence::Undo, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->undo();
    });
    editMenu->addAction(tr("&Redo"), QKeySequence::Redo, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->redo();
    });
    editMenu->addSeparator();
    editMenu->addAction(tr("Cu&t"), QKeySequence::Cut, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->cut();
    });
    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->copy();
    });
    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->paste();
    });
    editMenu->addAction(tr("Select &All"), QKeySequence::SelectAll, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->selectAll();
    });
    editMenu->addSeparator();
    editMenu->addAction(tr("&Find..."), QKeySequence::Find, this, [this] { showFind(false); });
    editMenu->addAction(tr("&Replace..."), QKeySequence::Replace, this,
                        [this] { showFind(true); });

    // View ------------------------------------------------------------------
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("&Settings..."), QKeySequence(QStringLiteral("Ctrl+,")), this,
                        &MainWindow::showSettings);

    // Plugins menu (placeholder; the host adds submenus into it)
    m_pluginMenu = menuBar()->addMenu(tr("&Plugins"));

    // Help -----------------------------------------------------------------
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&Help"), QKeySequence::HelpContents, this, &MainWindow::showHelp);
    helpMenu->addAction(tr("Run &Self Test"), this, &MainWindow::runSelfTest);
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About"), QKeySequence::WhatsThis, this, &MainWindow::showAbout);

    // Tab shortcuts ----------------------------------------------------------
    auto *prevTabAction = new QAction(this);
    prevTabAction->setShortcut(QKeySequence(QStringLiteral("F1")));
    connect(prevTabAction, &QAction::triggered, this, &MainWindow::prevTab);
    addAction(prevTabAction);

    auto *nextTabAction = new QAction(this);
    nextTabAction->setShortcut(QKeySequence(QStringLiteral("F2")));
    connect(nextTabAction, &QAction::triggered, this, &MainWindow::nextTab);
    addAction(nextTabAction);

    auto *completeAction = new QAction(this);
    completeAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Space")));
    connect(completeAction, &QAction::triggered, this, [this] {
        if (auto *t = currentEditorTab())
            t->editor()->requestCompletionPublic();
    });
    addAction(completeAction);

    auto *escAction = new QAction(this);
    escAction->setShortcut(Qt::Key_Escape);
    connect(escAction, &QAction::triggered, this, [this] {
        if (auto *t = currentEditorTab())
            t->hideFindBar();
    });
    addAction(escAction);
}

void MainWindow::createToolbar()
{
    auto *toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);
    toolbar->setObjectName(QStringLiteral("mainToolbar"));

    m_encodingCombo = new QComboBox(toolbar);
    fillEncodingCombo();
    m_encodingCombo->setToolTip(tr("Encoding of the current tab"));
    toolbar->addWidget(new QLabel(tr("Encoding:"), toolbar));
    toolbar->addWidget(m_encodingCombo);

    m_bomCheck = new QCheckBox(tr("BOM"), toolbar);
    m_bomCheck->setToolTip(tr("Write a BOM when saving (UTF-8/16/32)"));
    toolbar->addWidget(m_bomCheck);

    connect(m_encodingCombo, &QComboBox::activated, this, &MainWindow::onEncodingChanged);
    connect(m_bomCheck, &QCheckBox::toggled, this, &MainWindow::onBomToggled);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::fillEncodingCombo()
{
    m_encodingCombo->clear();
    // Item data carries the real EncodingService id; the AUTO id drives
    // BOM/heuristic detection on open.
    m_encodingCombo->addItem(tr("auto-detect"), EncodingService::AUTO);
    for (const auto &info : EncodingService::instance().encodings())
        m_encodingCombo->addItem(info.id, info.id);
    const QString def =
        AppSettings::instance()->value(AppSettings::kEncodingDefaultOpen).toString();
    int idx = m_encodingCombo->findData(def);
    if (idx < 0)
        idx = 0;
    m_encodingCombo->setCurrentIndex(idx);
}

QMenu *MainWindow::pluginMenuPlaceholder(const QString &title)
{
    // Plugins get their own top-level menus; merge into one if it exists.
    const QList<QAction *> actions = m_pluginMenu->actions();
    for (QAction *a : actions) {
        if (a->menu() && a->menu()->title().compare(title, Qt::CaseInsensitive) == 0)
            return a->menu();
    }
    return m_pluginMenu->addMenu(title);
}

QString MainWindow::currentDocumentTextForPlugins() const
{
    if (auto *t = currentEditorTab())
        return t->editor()->toPlainText();
    return QString();
}

QString MainWindow::currentDocumentPathForPlugins() const
{
    const DocView *v = currentView();
    return v ? v->filePath() : QString();
}

void MainWindow::showStatusMessageForPlugins(const QString &message, int msecs)
{
    statusBar()->showMessage(message, msecs);
}

// ---------------------------------------------------------------------
// Tab management

void MainWindow::connectTab(DocView *view)
{
    connect(view, &DocView::changed, this, [this, view] {
        const int idx = m_tabWidget->indexOf(view);
        if (idx >= 0) {
            m_tabWidget->setTabText(idx, view->title());
            m_tabWidget->setTabToolTip(idx, view->toolTip());
        }
        updateStatusBar();
    });
    connect(view, &DocView::statusMessage, this, [this](const QString &msg) {
        statusBar()->showMessage(msg, 6000);
    });
    if (auto *tab = qobject_cast<EditorTab *>(view)) {
        connect(tab, &EditorTab::cursorMoved, this,
                [this](int line, int col) {
                    statusBar()->showMessage(tr("Line %1, Column %2").arg(line).arg(col));
                });
        // After a load the effective encoding may have changed (detection);
        // sync the toolbar combo with it.
        connect(tab, &EditorTab::infoChanged, this, [this] {
            if (auto *t = currentEditorTab()) {
                const int idx = m_encodingCombo->findData(t->encoding());
                if (idx >= 0)
                    m_encodingCombo->setCurrentIndex(idx);
            }
            updateStatusBar();
        });
    }
}

EditorTab *MainWindow::currentEditorTab() const
{
    return qobject_cast<EditorTab *>(m_tabWidget->currentWidget());
}

DocView *MainWindow::currentView() const
{
    return qobject_cast<DocView *>(m_tabWidget->currentWidget());
}

int MainWindow::openEditorTabFor(const QString &path)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *view = qobject_cast<DocView *>(m_tabWidget->widget(i));
        if (view && view->filePath() == path && qobject_cast<EditorTab *>(view))
            return i;
    }
    return -1;
}

void MainWindow::newFile()
{
    auto *tab = new EditorTab(this);
    connectTab(tab);
    m_tabWidget->addTab(tab, tab->title());
    m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
    tab->editor()->setFocus();
    updateStatusBar();
}

void MainWindow::startNew()
{
    newFile();
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                      tr("All Files (*)"));
    if (!path.isEmpty())
        openPath(path);
}

void MainWindow::openPath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        QMessageBox::warning(this, tr("Error"), tr("File does not exist: %1").arg(path));
        return;
    }

    const int existing = openEditorTabFor(info.absoluteFilePath());
    if (existing >= 0) {
        m_tabWidget->setCurrentIndex(existing);
        return;
    }

    // Capture the requested encoding BEFORE the tab is added: switching to
    // the new tab syncs the combo to the tab's current encoding.
    const QString encoding = m_encodingCombo->currentData().toString();

    auto *tab = new EditorTab(this);
    connectTab(tab);
    m_tabWidget->addTab(tab, tr("[0%] %1").arg(info.fileName()));
    m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);

    tab->load(info.absoluteFilePath(), encoding);
    tab->editor()->setFocus();
    updateStatusBar();
}

void MainWindow::openInHex(const QString &path)
{
    QString target = path;
    if (target.isEmpty()) {
        if (auto *t = currentEditorTab())
            target = t->filePath();
    }
    if (target.isEmpty()) {
        target = QFileDialog::getOpenFileName(this, tr("Open File in Hex Editor"),
                                              QString(), tr("All Files (*)"));
        if (target.isEmpty())
            return;
    }

    // Reuse an existing hex tab for the same file.
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *hex = qobject_cast<HexEditor *>(m_tabWidget->widget(i));
        if (hex && hex->filePath() == target) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    auto *hex = new HexEditor(target, this);
    connectTab(hex);
    m_tabWidget->addTab(hex, hex->title());
    m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
}

void MainWindow::startOpen(const QString &path)
{
    openPath(path);
}

void MainWindow::saveFile()
{
    DocView *view = currentView();
    if (!view)
        return;
    view->save();
}

void MainWindow::saveFileAs()
{
    DocView *view = currentView();
    if (!view)
        return;
    view->saveAs();
}

void MainWindow::reloadTab()
{
    auto *tab = currentEditorTab();
    if (!tab)
        return;
    if (tab->isDirty()
        && QMessageBox::question(this, tr("Reload"),
                                 tr("Discard unsaved changes and reload?"))
               != QMessageBox::Yes)
        return;
    tab->reload();
}

void MainWindow::closeTab(int index)
{
    auto *view = qobject_cast<DocView *>(m_tabWidget->widget(index));
    if (!view)
        return;
    if (!confirmDiscard(view))
        return;
    m_tabWidget->removeTab(index);
    view->deleteLater();
    updateStatusBar();
}

void MainWindow::closeCurrentTab()
{
    const int idx = m_tabWidget->currentIndex();
    if (idx >= 0)
        closeTab(idx);
}

bool MainWindow::confirmDiscard(DocView *view)
{
    if (!view->isDirty())
        return true;
    const auto answer = QMessageBox::question(
        this, tr("Unsaved changes"),
        tr("\"%1\" has unsaved changes. Discard them?").arg(view->title()),
        QMessageBox::Discard | QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

void MainWindow::nextTab()
{
    const int n = m_tabWidget->count();
    if (n)
        m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % n);
}

void MainWindow::prevTab()
{
    const int n = m_tabWidget->count();
    if (n)
        m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() - 1 + n) % n);
}

void MainWindow::showFind(bool withReplace)
{
    auto *tab = currentEditorTab();
    if (!tab)
        return;
    tab->activateFind(withReplace);
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(m_pluginHost, this);
    dialog.exec();
    // Live application happens through AppSettings::settingsChanged ->
    // applySettingsToTabs; call once more for plugins/hex defaults.
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *view = qobject_cast<DocView *>(m_tabWidget->widget(i)))
            view->applySettings();
    }
}

void MainWindow::applySettingsToTabs(const QStringList &keys)
{
    Q_UNUSED(keys);
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *view = qobject_cast<DocView *>(m_tabWidget->widget(i)))
            view->applySettings();
    }
}

void MainWindow::showHelp()
{
    QFile help(QStringLiteral(":/help.txt"));
    if (!help.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Help file missing"));
        return;
    }
    auto *tab = new EditorTab(this);
    connectTab(tab);
    m_tabWidget->addTab(tab, tab->title());
    m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
    tab->loadTextDirect(tr("Help"),
                        QString::fromUtf8(help.readAll()));
    updateStatusBar();
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this, tr("About Apple_Cat"),
        tr("<b>Apple_Cat %1</b><br>A tabbed text editor with async loading, "
           "an embedded hex editor, multi-source completion and plugins.<br><br>"
           "Origin: github.com/KamisatoAyaka7/File_Creater<br>Developed by szy.")
            .arg(QString::fromLatin1(AppleCat::Core::coreVersion())));
}

void MainWindow::runSelfTest()
{
    AppleCat::Core::SelfTest test;
    const QStringList lines = test.runAll();
    int failCount = 0;
    for (const QString &l : lines) {
        if (l.startsWith(QLatin1String("FAIL")))
            ++failCount;
    }

    auto *tab = new EditorTab(this);
    connectTab(tab);
    m_tabWidget->addTab(tab, tab->title());
    m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
    QString text = tr("Self test %1\n\n")
                       .arg(test.allPassed() ? tr("PASSED") : tr("FAILED"));
    for (const QString &l : lines)
        text += l + QLatin1Char('\n');
    tab->loadTextDirect(tr("Self test"), text);
    statusBar()->showMessage(tr("Self test: %1 checks, %2 failed")
                                 .arg(lines.size())
                                 .arg(failCount),
                             10000);
    updateStatusBar();
}

// ---------------------------------------------------------------------
// Toolbar callbacks

void MainWindow::onEncodingChanged(int)
{
    auto *tab = currentEditorTab();
    if (!tab)
        return;
    tab->setEncodingAndReload(m_encodingCombo->currentData().toString());
}

void MainWindow::onBomToggled()
{
    auto *tab = currentEditorTab();
    if (!tab)
        return;
    tab->setBomWanted(m_bomCheck->isChecked());
    statusBar()->showMessage(m_bomCheck->isChecked() ? tr("A BOM will be written on save")
                                                     : tr("No BOM on save"),
                             3000);
}

// ---------------------------------------------------------------------
// Status bar / tab sync

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    const DocView *view = currentView();
    if (auto *tab = currentEditorTab()) {
        m_encodingCombo->setEnabled(true);
        m_bomCheck->setEnabled(true);
        m_bomCheck->setChecked(tab->bomWanted());
        const int idx = m_encodingCombo->findData(tab->encoding());
        if (idx >= 0)
            m_encodingCombo->setCurrentIndex(idx);
    } else {
        m_encodingCombo->setEnabled(false);
        m_bomCheck->setEnabled(false);
    }
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    const DocView *view = currentView();
    if (!view) {
        statusBar()->showMessage(tr("Ready"));
        return;
    }
    if (auto *tab = currentEditorTab()) {
        const QTextCursor cursor = tab->editor()->textCursor();
        statusBar()->showMessage(tr("Line %1, Column %2  ·  %3  ·  %4")
                                     .arg(cursor.blockNumber() + 1)
                                     .arg(tab->editor()->columnNumber())
                                     .arg(EncodingService::instance().displayName(tab->encoding()))
                                     .arg(view->isDirty() ? tr("modified") : tr("saved")));
    } else if (view->isLoading()) {
        statusBar()->showMessage(tr("Loading %1...").arg(QFileInfo(view->filePath()).fileName()));
    } else {
        statusBar()->showMessage(
            tr("Hex view  ·  %1  ·  %2")
                .arg(QFileInfo(view->filePath()).fileName(),
                     view->isDirty() ? tr("modified") : tr("saved")));
    }
}

// ---------------------------------------------------------------------
// Drag & drop / close

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            openPath(url.toLocalFile());
    }
    event->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *view = qobject_cast<DocView *>(m_tabWidget->widget(i));
        if (view && view->isDirty() && !confirmDiscard(view)) {
            event->ignore();
            return;
        }
    }
    AppSettings::instance()->setValue(AppSettings::kWindowGeometry, saveGeometry());
    event->accept();
}

} // namespace AppleCat::Gui
