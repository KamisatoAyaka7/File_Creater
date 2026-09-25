#include "pluginhost.h"

#include "appsettings.h"
#include "completion.h"
#include "mainwindow.h"

#include <QAction>
#include <QDirIterator>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPluginLoader>

namespace AppleCat::Gui {

using AppleCat::Core::AppSettings;

// ---------------------------------------------------------------------
// AppContext

QMenu *AppContext::menu(const QString &title)
{
    return m_host->ensureMenu(title);
}

QAction *AppContext::addAction(QMenu *menu, const QString &text, QObject *receiver,
                               const char *slot)
{
    if (!menu)
        return nullptr;
    QAction *action = menu->addAction(text, receiver, slot);
    return action;
}

QString AppContext::currentDocumentText() const
{
    return m_host->currentDocumentText();
}

QString AppContext::currentDocumentPath() const
{
    return m_host->currentDocumentPath();
}

void AppContext::showStatusMessage(const QString &message, int msecs)
{
    m_host->showStatusMessage(message, msecs);
}

void AppContext::showMessage(const QString &title, const QString &text)
{
    m_host->showMessage(title, text);
}

void AppContext::registerCompletionProvider(AppleCat::ICompletionProvider *provider)
{
    AppleCat::Core::CompletionEngine::instance()->registerProvider(provider);
}

void AppContext::unregisterCompletionProvider(AppleCat::ICompletionProvider *provider)
{
    AppleCat::Core::CompletionEngine::instance()->unregisterProvider(provider);
}

QVariant AppContext::setting(const QString &key, const QVariant &defaultValue) const
{
    if (auto *s = AppleCat::Core::AppSettings::instance())
        return s->value(key, defaultValue);
    return defaultValue;
}

// ---------------------------------------------------------------------
// PluginHost

PluginHost::PluginHost(QObject *parent)
    : QObject(parent), m_context(this)
{
}

QMenu *PluginHost::ensureMenu(const QString &title)
{
    auto it = m_menus.find(title);
    if (it != m_menus.end())
        return it.value();
    QMenu *menu = nullptr;
    if (m_mainWindow)
        menu = m_mainWindow->pluginMenuPlaceholder(title);
    if (!menu) {
        // Parent to the main window (a QWidget) when available so the menu
        // is owned and destroyed properly.
        menu = new QMenu(title, m_mainWindow);
    }
    m_menus.insert(title, menu);
    return menu;
}

QString PluginHost::currentDocumentText() const
{
    return m_mainWindow ? m_mainWindow->currentDocumentTextForPlugins() : QString();
}

QString PluginHost::currentDocumentPath() const
{
    return m_mainWindow ? m_mainWindow->currentDocumentPathForPlugins() : QString();
}

void PluginHost::showStatusMessage(const QString &message, int msecs)
{
    if (m_mainWindow)
        m_mainWindow->showStatusMessageForPlugins(message, msecs);
}

void PluginHost::showMessage(const QString &title, const QString &text)
{
    QMessageBox::information(m_mainWindow, title, text);
}

void PluginHost::loadAll()
{
    const auto *s = AppleCat::Core::AppSettings::instance();
    const QStringList disabled = s->value(AppSettings::kPluginsDisabled).toStringList();

    QStringList dirs;
    if (s)
        dirs = s->pluginSearchPaths();

    QStringList candidates;
    for (const QString &dir : dirs) {
        QDirIterator it(dir, {QStringLiteral("*.dll"), QStringLiteral("*.so"),
                              QStringLiteral("*.dylib")},
                        QDir::Files);
        while (it.hasNext())
            candidates.append(it.next());
    }

    for (const QString &path : candidates) {
        const bool enabled = !disabled.contains(QFileInfo(path).fileName());
        loadPlugin(path, enabled);
    }
    emit pluginsChanged();
}

void PluginHost::loadPlugin(const QString &path, bool enabled)
{
    LoadedPlugin p;
    p.path = path;
    p.enabled = enabled;

    if (!enabled) {
        p.error = tr("disabled in settings");
        m_plugins.append(p);
        return;
    }

    auto *loader = new QPluginLoader(path, this);
    // One attempt at loading; a broken plugin reports through errorString().
    if (!loader->load()) {
        p.error = loader->errorString();
        delete loader;
        m_plugins.append(p);
        return;
    }

    QObject *root = loader->instance();
    AppleCat::IPlugin *plugin = qobject_cast<AppleCat::IPlugin *>(root);
    if (!plugin) {
        p.error = tr("not an AppleCat IPlugin (missing Q_PLUGIN_METADATA?)");
        loader->unload();
        delete loader;
        m_plugins.append(p);
        return;
    }

    p.loader = loader;
    p.plugin = plugin;
    p.name = plugin->name();
    p.version = plugin->version();
    p.vendor = plugin->vendor();
    p.description = plugin->description();

    if (!plugin->initialize(&m_context)) {
        p.error = tr("initialize() returned false");
        plugin->shutdown();
        loader->unload();
        delete loader;
        p.plugin = nullptr;
        m_plugins.append(p);
        return;
    }

    p.active = true;
    m_plugins.append(p);
}

bool PluginHost::setEnabled(const QString &name, bool enabled)
{
    for (LoadedPlugin &p : m_plugins) {
        const QString key = QFileInfo(p.path).fileName();
        if (key != name && p.name != name)
            continue;
        if (p.enabled == enabled)
            return true;

        auto *s = AppleCat::Core::AppSettings::instance();
        QStringList disabled = s->value(AppSettings::kPluginsDisabled).toStringList();

        if (!enabled && p.active) {
            p.plugin->shutdown();
            if (p.loader) {
                p.loader->unload();
                delete p.loader;
            }
            p.loader = nullptr;
            p.plugin = nullptr;
            p.active = false;
            p.error = tr("disabled in settings");
        } else if (enabled) {
            // Fresh attempt; rebuild the entry.
            const QString path = p.path;
            disabled.removeAll(key);
            s->setValue(AppSettings::kPluginsDisabled, disabled);
            int idx = -1;
            for (int i = 0; i < m_plugins.size(); ++i) {
                if (m_plugins[i].path == p.path) {
                    idx = i;
                    break;
                }
            }
            if (idx >= 0)
                m_plugins.removeAt(idx);
            loadPlugin(path, true);
            emit pluginsChanged();
            return true;
        }

        if (!disabled.contains(key))
            disabled.append(key);
        s->setValue(AppSettings::kPluginsDisabled, disabled);
        emit pluginsChanged();
        return true;
    }
    return false;
}

void PluginHost::unloadPlugin(LoadedPlugin &p)
{
    if (p.active && p.plugin)
        p.plugin->shutdown();
    if (p.loader) {
        p.loader->unload();
        delete p.loader;
    }
    p.loader = nullptr;
    p.plugin = nullptr;
    p.active = false;
}

} // namespace AppleCat::Gui
