// Multi-source completion engine.
//
// Sources (each independently toggleable in the settings):
//   * keywords  - from the active syntax definition
//   * snippets  - from snippets.json (per-language + global "*")
//   * doc words - words extracted from the current document (maintained by
//                 the editor, debounced)
//   * plugins   - ICompletionProvider implementations registered at runtime
//
// Ranking: exact prefix match beats fuzzy subsequence match; shorter
// candidates rank first. Results are capped so the popup stays snappy.

#pragma once

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QHash>

#include "applecat_plugin.h"

namespace AppleCat::Core {

struct CompletionCandidate
{
    QString text;    // text to insert
    QString detail;  // shown after the text in the popup
    int kind = 2;    // 0 keyword, 1 snippet, 2 doc word, 3 plugin
    int score = 0;
};

struct Snippet
{
    QString prefix;  // trigger word
    QString body;    // may contain ${cursor}
    QString detail;
};

class CompletionEngine : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        bool enabled = true;
        bool autoPopup = true;
        bool fuzzy = true;
        int minPrefix = 2;
        bool keywords = true;
        bool snippets = true;
        bool docWords = true;
    };

    static CompletionEngine *instance();
    void init(); // (re)load snippets from the search paths

    // Reads the current options from AppSettings.
    Options options() const;

    // docWords may be null. filePath selects language keywords/snippets.
    QList<CompletionCandidate> compute(const QString &prefix, const QString &filePath,
                                       const QSet<QString> *docWords) const;

    QList<Snippet> snippetsFor(const QString &filePath) const;

    void registerProvider(AppleCat::ICompletionProvider *provider);
    void unregisterProvider(AppleCat::ICompletionProvider *provider);
    QList<AppleCat::ICompletionProvider *> providers() const { return m_providers; }

    // Scoring helper (also exercised by the self test).
    // Returns -1 when the candidate does not match the prefix.
    static int matchScore(const QString &candidate, const QString &prefix, bool fuzzy);

    // Word extraction used by the editor's document-word cache.
    static QSet<QString> extractWords(const QString &text, int maxWords);

    static constexpr int kMaxResults = 50;

private:
    explicit CompletionEngine(QObject *parent = nullptr);
    void loadSnippetsFrom(const QString &path);

    QHash<QString, QList<Snippet>> m_snippets; // key: lower extension or "*"
    QList<AppleCat::ICompletionProvider *> m_providers;
};

} // namespace AppleCat::Core
