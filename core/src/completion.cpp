#include "completion.h"

#include "appsettings.h"
#include "syntaxrepository.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <algorithm>
#include <functional>

namespace AppleCat::Core {

static CompletionEngine *g_instance = nullptr;

CompletionEngine *CompletionEngine::instance()
{
    if (!g_instance) {
        g_instance = new CompletionEngine();
        g_instance->init();
    }
    return g_instance;
}

CompletionEngine::CompletionEngine(QObject *parent)
    : QObject(parent)
{
}

void CompletionEngine::init()
{
    m_snippets.clear();
    const QStringList paths = AppSettings::instance()
                                  ? AppSettings::instance()->snippetSearchPaths()
                                  : QStringList{QStringLiteral(":/snippets.json")};
    // Highest priority first; the first snippet with a given (lang, prefix)
    // wins because later files skip already-present triggers.
    for (const QString &path : paths)
        loadSnippetsFrom(path);
}

void CompletionEngine::loadSnippetsFrom(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();
    for (auto langIt = root.begin(); langIt != root.end(); ++langIt) {
        const QString lang = langIt.key().toLower();
        const QJsonArray arr = langIt.value().toArray();
        QList<Snippet> &list = m_snippets[lang];
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            Snippet s;
            s.prefix = o.value(QStringLiteral("prefix")).toString();
            s.body = o.value(QStringLiteral("body")).toString();
            s.detail = o.value(QStringLiteral("detail")).toString();
            if (s.prefix.isEmpty() || s.body.isEmpty())
                continue;
            // A higher-priority file already provided this trigger.
            const bool dup = std::any_of(list.cbegin(), list.cend(),
                                         [&](const Snippet &e) { return e.prefix == s.prefix; });
            if (dup)
                continue;
            list.append(s);
        }
    }
}

CompletionEngine::Options CompletionEngine::options() const
{
    const AppSettings *s = AppSettings::instance();
    if (!s)
        return {};
    Options o;
    o.enabled = s->value(AppSettings::kCompletionEnabled).toBool();
    o.autoPopup = s->value(AppSettings::kCompletionAutoPopup).toBool();
    o.fuzzy = s->value(AppSettings::kCompletionFuzzy).toBool();
    o.minPrefix = s->value(AppSettings::kCompletionMinPrefix).toInt();
    o.keywords = s->value(AppSettings::kCompletionKeywords).toBool();
    o.snippets = s->value(AppSettings::kCompletionSnippets).toBool();
    o.docWords = s->value(AppSettings::kCompletionDocWords).toBool();
    return o;
}

int CompletionEngine::matchScore(const QString &candidate, const QString &prefix,
                                 bool fuzzy)
{
    if (prefix.isEmpty() || candidate.size() < prefix.size())
        return -1;

    if (candidate.startsWith(prefix, Qt::CaseInsensitive)) {
        // Exact prefix: shorter candidates first; exact-case bonus.
        const int score = 100000 - candidate.size();
        return candidate.startsWith(prefix, Qt::CaseSensitive) ? score + 5000 : score;
    }

    if (!fuzzy)
        return -1;

    // Fuzzy subsequence: every prefix char must appear in order.
    int ci = 0, matched = 0, last = -1;
    int span = 0, first = -1;
    for (; ci < candidate.size() && matched < prefix.size(); ++ci) {
        if (candidate[ci].toLower() == prefix[matched].toLower()) {
            if (first < 0)
                first = ci;
            span = ci - first + 1;
            last = ci;
            ++matched;
        }
    }
    if (matched < prefix.size())
        return -1;
    // Tighter (more compact) matches first.
    return 50000 - span * 10 - (candidate.size() - span);
}

QSet<QString> CompletionEngine::extractWords(const QString &text, int maxWords)
{
    QSet<QString> words;
    static const QRegularExpression re(
        QStringLiteral("\\w{2,40}"),
        QRegularExpression::UseUnicodePropertiesOption);
    auto it = re.globalMatch(text);
    while (it.hasNext() && words.size() < maxWords) {
        const QString w = it.next().captured();
        // Skip pure numbers.
        bool onlyDigits = true;
        for (const QChar &c : w) {
            if (!c.isDigit()) {
                onlyDigits = false;
                break;
            }
        }
        if (!onlyDigits)
            words.insert(w);
    }
    return words;
}

QList<Snippet> CompletionEngine::snippetsFor(const QString &filePath) const
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    QList<Snippet> result = m_snippets.value(QStringLiteral("*"));
    result += m_snippets.value(ext);
    return result;
}

QList<CompletionCandidate> CompletionEngine::compute(const QString &prefix,
                                                     const QString &filePath,
                                                     const QSet<QString> *docWords) const
{
    QList<CompletionCandidate> out;
    if (prefix.isEmpty())
        return out;
    const Options opt = options();
    if (!opt.enabled)
        return out;

    auto addCandidates = [&](const QStringList &texts, int kind, const QString &detailBase,
                             const std::function<QString(const QString &)> &detailOf) {
        for (const QString &t : texts) {
            const int score = matchScore(t, prefix, opt.fuzzy);
            if (score < 0)
                continue;
            CompletionCandidate c;
            c.text = t;
            c.kind = kind;
            c.score = score;
            c.detail = detailOf ? detailOf(t) : detailBase;
            out.append(c);
        }
    };

    // 1. Keywords from the syntax definition.
    if (opt.keywords) {
        auto def = SyntaxRepository::instance().definitionForFile(filePath);
        if (def) {
            QStringList words;
            words.reserve(def->keywords.size());
            for (const auto &kw : def->keywords)
                words.append(kw.first);
            addCandidates(words, 0, tr("keyword"), nullptr);
        }
    }

    // 2. Snippets (kind 1); detail shows the snippet body's first line.
    if (opt.snippets) {
        const QList<Snippet> snips = snippetsFor(filePath);
        for (const Snippet &s : snips) {
            const int score = matchScore(s.prefix, prefix, opt.fuzzy);
            if (score < 0)
                continue;
            CompletionCandidate c;
            c.text = s.prefix;
            c.kind = 1;
            c.score = score + 200; // slight edge over plain words
            QString firstLine = s.body;
            const int nl = firstLine.indexOf(QLatin1Char('\n'));
            if (nl >= 0)
                firstLine.truncate(nl);
            c.detail = s.detail.isEmpty() ? tr("snippet: %1").arg(firstLine)
                                          : s.detail;
            c.detail = tr("snippet: %1").arg(c.detail);
            out.append(c);
        }
    }

    // 3. Words from the current document.
    if (opt.docWords && docWords && !docWords->isEmpty()) {
        addCandidates(docWords->values(), 2, QString(), nullptr);
    }

    // 4. Plugin providers.
    for (AppleCat::ICompletionProvider *p : m_providers) {
        const QList<AppleCat::CompletionItemView> items = p->completions(prefix, filePath);
        for (const AppleCat::CompletionItemView &item : items) {
            if (item.text.isEmpty())
                continue;
            const int score = matchScore(item.text, prefix, opt.fuzzy);
            if (score < 0)
                continue;
            CompletionCandidate c;
            c.text = item.text;
            c.kind = 3;
            c.score = score + 100;
            c.detail = item.detail.isEmpty() ? tr("plugin: %1").arg(p->providerId())
                                             : item.detail;
            out.append(c);
        }
    }

    std::sort(out.begin(), out.end(), [](const CompletionCandidate &a,
                                         const CompletionCandidate &b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.text < b.text;
    });

    // Deduplicate identical insert texts, keeping the best score/kind.
    QSet<QString> seen;
    QList<CompletionCandidate> deduped;
    deduped.reserve(qMin(out.size(), kMaxResults));
    for (const CompletionCandidate &c : std::as_const(out)) {
        const QString key = c.text + QLatin1Char('#') + QString::number(c.kind);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        deduped.append(c);
        if (deduped.size() >= kMaxResults)
            break;
    }
    return deduped;
}

void CompletionEngine::registerProvider(AppleCat::ICompletionProvider *provider)
{
    if (provider && !m_providers.contains(provider))
        m_providers.append(provider);
}

void CompletionEngine::unregisterProvider(AppleCat::ICompletionProvider *provider)
{
    m_providers.removeAll(provider);
}

} // namespace AppleCat::Core
