// Main window: tabs of DocViews (EditorTab / HexEditor), toolbar menus,
// encoding combo, recent files, drag & drop, close-with-unsaved guard.

#pragma once

#include <QCheckBox>
#include <QMainWindow>

class QComboBox;
class QLabel;
class QMenu;
class QTabWidget;

namespace AppleCat::Gui {

class DocView;
class EditorTab;
class PluginHost;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void startOpen(const QString &path);
    void startNew();

    // For the plugin host -------------------------------------------------
    QMenu *pluginMenuPlaceholder(const QString &title);
    QString currentDocumentTextForPlugins() const;
    QString currentDocumentPathForPlugins() const;
    void showStatusMessageForPlugins(const QString &message, int msecs);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void openPath(const QString &path);
    void openInHex(const QString &path);
    void saveFile();
    void saveFileAs();
    void reloadTab();
    void closeTab(int index);
    void closeCurrentTab();
    void nextTab();
    void prevTab();
    void showFind(bool withReplace);
    void showSettings();
    void showHelp();
    void showAbout();
    void runSelfTest();
    void updateStatusBar();
    void onTabChanged(int index);
    void onEncodingChanged(int index);
    void onBomToggled();
    void applySettingsToTabs(const QStringList &keys);

private:
    void createMenus();
    void createToolbar();
    void createStatusBar();
    void connectTab(DocView *view);
    EditorTab *currentEditorTab() const;
    DocView *currentView() const;
    int openEditorTabFor(const QString &path); // -1 if not open
    void fillEncodingCombo();
    bool confirmDiscard(DocView *view);

    QTabWidget *m_tabWidget = nullptr;
    QComboBox *m_encodingCombo = nullptr;
    QCheckBox *m_bomCheck = nullptr;
    QMenu *m_recentMenu = nullptr;
    QMenu *m_pluginMenu = nullptr;

    PluginHost *m_pluginHost = nullptr;
};

} // namespace AppleCat::Gui
