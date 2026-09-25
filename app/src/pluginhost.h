// Plugin host: loads IPlugin dynamic libraries, provides the IAppContext
// implementation the plugins talk to, tracks enable/disable state in the
// settings, and wires plugin completion providers into the engine.

#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include "applecat_plugin.h"

class QMenu;
class QPluginLoader;

namespace AppleCat::Core {
class CompletionEngine;
}

namespace AppleCat::Gui {

class PluginHost;

class AppContext final : public AppleCat::IAppContext
{
public:
    explicit AppContext(PluginHost *host)
        : m_host(host)
    {
    }

    QMenu *menu(const QString &title) override;
    QAction *addAction(QMenu *menu, const QString &text, QObject *receiver,
                       const char *slot) override;
    QString currentDocumentText() const override;
    QString currentDocumentPath() const override;
    void showStatusMessage(const QString &message, int msecs) override;
    void showMessage(const QString &title, const QString &text) override;
    void registerCompletionProvider(AppleCat::ICompletionProvider *provider) override;
    void unregisterCompletionProvider(AppleCat::ICompletionProvider *provider) override;
    QVariant setting(const QString &key, const QVariant &defaultValue) const override;

private:
    PluginHost *m_host;
};

struct LoadedPlugin
{
    QString path;
    QPluginLoader *loader = nullptr;
    AppleCat::IPlugin *plugin = nullptr;
    QString name;
    QString version;
    QString vendor;
    QString description;
    QString error;
    bool enabled = true; // intended state (settings)
    bool active = false; // successfully initialized
};

class PluginHost : public QObject
{
    Q_OBJECT

public:
    explicit PluginHost(QObject *parent = nullptr);

    // Scans plugin folders, loads enabled plugins. Call after the main
    // window is constructed (plugins add menus right away).
    void loadAll();

    QList<LoadedPlugin> plugins() const { return m_plugins; }

    // Enables/disables a plugin. Disabling unloads it (shutdown()) if it
    // was active; enabling tries to load it right away.
    bool setEnabled(const QString &name, bool enabled);

    QMenu *ensureMenu(const QString &title);

    // AppContext callbacks provided by the main window.
    void setMainWindow(class MainWindow *win) { m_mainWindow = win; }

    QString currentDocumentText() const;
    QString currentDocumentPath() const;
    void showStatusMessage(const QString &message, int msecs);
    void showMessage(const QString &title, const QString &text);

signals:
    void pluginsChanged();

private:
    void loadPlugin(const QString &path, bool enabled);
    void unloadPlugin(LoadedPlugin &p);

    QList<LoadedPlugin> m_plugins;
    QHash<QString, QMenu *> m_menus; // title -> menu (plugins' menus)
    MainWindow *m_mainWindow = nullptr;
    AppContext m_context;
};

} // namespace AppleCat::Gui
