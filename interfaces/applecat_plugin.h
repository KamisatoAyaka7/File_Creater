// Apple_Cat 4.x plugin SDK.
//
// A plugin is a shared library that exports exactly one QObject derived from
// AppleCat::IPlugin and annotated with:
//
//     Q_PLUGIN_METADATA(IID "org.szy.AppleCat.IPlugin/4.0" FILE "metadata.json")
//
// The library only needs to link Qt (Core/Gui/Widgets) and this header-only
// SDK; it must NOT link the application or its static core library.
//
// metadata.json (embedded via Q_PLUGIN_METADATA):
//     { "name": "...", "version": "...", "vendor": "...", "description": "..." }

#pragma once

#include <QString>
#include <QVariant>
#include <QList>

class QMenu;
class QAction;
class QObject;

namespace AppleCat {

// A completion item as seen by the completion engine.
struct CompletionItemView
{
    QString text;    // text to insert
    QString detail;  // optional description shown in the popup
    int kind = 3;    // 0 keyword, 1 snippet, 2 word, 3 plugin
};

// Implement this to feed the editor's completion popup.
class ICompletionProvider
{
public:
    virtual ~ICompletionProvider() = default;

    virtual QString providerId() const = 0;

    // Called on every keystroke while the completion popup could show.
    // Return an empty list to contribute nothing.
    virtual QList<CompletionItemView> completions(const QString &prefix,
                                                  const QString &filePath) = 0;
};

// Services the host application exposes to plugins.
class IAppContext
{
public:
    virtual ~IAppContext() = default;

    // Returns (creating if needed) a top-level menu with the given title.
    virtual QMenu *menu(const QString &title) = 0;

    // Adds an action to `menu`; when triggered, `slot` (a SLOT(...) string)
    // is invoked on `receiver`.
    virtual QAction *addAction(QMenu *menu, const QString &text,
                               QObject *receiver, const char *slot) = 0;

    virtual QString currentDocumentText() const = 0;
    virtual QString currentDocumentPath() const = 0;

    virtual void showStatusMessage(const QString &message, int msecs = 4000) = 0;
    virtual void showMessage(const QString &title, const QString &text) = 0;

    virtual void registerCompletionProvider(ICompletionProvider *provider) = 0;
    virtual void unregisterCompletionProvider(ICompletionProvider *provider) = 0;

    // Reads a value from the application settings (see qt6/README.md for keys).
    virtual QVariant setting(const QString &key,
                             const QVariant &defaultValue = QVariant()) const = 0;
};

// The plugin entry point.
class IPlugin
{
public:
    virtual ~IPlugin() = default;

    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString vendor() const = 0;
    virtual QString description() const = 0;

    // Called once at startup. Return false to signal failure (the plugin is
    // then unloaded and reported as failed in the settings dialog).
    virtual bool initialize(IAppContext *context) = 0;

    // Called at shutdown; may be skipped if the process exits abruptly.
    virtual void shutdown() = 0;
};

} // namespace AppleCat

Q_DECLARE_INTERFACE(AppleCat::IPlugin, "org.szy.AppleCat.IPlugin/4.0")
