// Example Apple_Cat plugin: adds a Plugins > Statistics menu action that
// reports character/word/line counts of the current document, and a
// completion provider offering markdown-ish snippets.

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPluginLoader>
#include <QRegularExpression>
#include <QStringList>
#include <QtPlugin>

#include "applecat_plugin.h"

using namespace AppleCat;

class StatsPlugin final : public QObject, public IPlugin, public ICompletionProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.szy.AppleCat.IPlugin/4.0" FILE "metadata.json")
    Q_INTERFACES(AppleCat::IPlugin)

public:
    QString name() const override { return QStringLiteral("Statistics"); }
    QString version() const override { return QStringLiteral("1.0"); }
    QString vendor() const override { return QStringLiteral("szy"); }
    QString description() const override
    {
        return QStringLiteral("Adds a Statistics action (chars/words/lines of the "
                              "current document) and a few sample completions.");
    }

    bool initialize(IAppContext *context) override
    {
        m_context = context;

        QMenu *menu = context->menu(QStringLiteral("Tools"));
        if (!menu)
            return false;
        m_action = context->addAction(menu, QStringLiteral("Statistics && Count"),
                                      this, SLOT(showStats()));
        if (!m_action)
            return false;

        context->registerCompletionProvider(this);
        return true;
    }

    void shutdown() override
    {
        if (m_context) {
            m_context->unregisterCompletionProvider(this);
            m_context = nullptr;
        }
    }

    // ICompletionProvider --------------------------------------------------
    QString providerId() const override { return QStringLiteral("stats"); }

    QList<CompletionItemView> completions(const QString &prefix,
                                          const QString &filePath) override
    {
        Q_UNUSED(filePath);
        QList<CompletionItemView> out;
        const QStringList words = {
            QStringLiteral("stat-sum"), QStringLiteral("stat-average"),
            QStringLiteral("stat-count"),
        };
        for (const QString &w : words) {
            if (w.startsWith(prefix, Qt::CaseInsensitive)) {
                CompletionItemView item;
                item.text = w;
                item.detail = QStringLiteral("sample completion from Statistics plugin");
                item.kind = 3;
                out.append(item);
            }
        }
        return out;
    }

private slots:
    void showStats()
    {
        if (!m_context)
            return;
        const QString text = m_context->currentDocumentText();
        const int chars = text.size();
        const int words = static_cast<int>(
            text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).size());
        const int lines = text.count(QLatin1Char('\n')) + (chars ? 1 : 0);
        m_context->showMessage(
            QStringLiteral("Statistics"),
            QStringLiteral("Characters: %1\nWords: %2\nLines: %3\n\nPath: %4")
                .arg(chars)
                .arg(words)
                .arg(lines)
                .arg(m_context->currentDocumentPath()));
    }

private:
    IAppContext *m_context = nullptr;
    QAction *m_action = nullptr;
};

#include "statsplugin.moc"
