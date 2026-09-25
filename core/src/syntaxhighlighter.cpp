#include "syntaxhighlighter.h"

#include <QTextBlock>

namespace AppleCat::Core {

// Block states below InComment are unused; InComment + i marks "inside the
// i-th block comment pair".
static constexpr int InComment = 100;

RuleSyntaxHighlighter::RuleSyntaxHighlighter(
    QTextDocument *document, std::shared_ptr<const SyntaxDefinition> definition)
    : QSyntaxHighlighter(document)
{
    setDefinition(definition);
}

RuleSyntaxHighlighter::~RuleSyntaxHighlighter() = default;

void RuleSyntaxHighlighter::setDefinition(std::shared_ptr<const SyntaxDefinition> definition)
{
    m_definition = definition;
    rebuild();
    rehighlight();
}

void RuleSyntaxHighlighter::rebuild()
{
    m_keywordsRe = QRegularExpression();
    m_keywordColors.clear();
    m_commentFormat = QTextCharFormat();

    if (!m_definition)
        return;

    QStringList escaped;
    for (const auto &kw : m_definition->keywords) {
        escaped << QRegularExpression::escape(kw.first);
        m_keywordColors.insert(kw.first, kw.second);
    }
    if (!escaped.isEmpty()) {
        QString pattern = QStringLiteral("(?<!\\w)(?:") + escaped.join(QLatin1Char('|'))
                          + QStringLiteral(")(?!\\w)");
        QRegularExpression::PatternOptions opts = QRegularExpression::UseUnicodePropertiesOption;
        if (!m_definition->caseSensitiveKeywords)
            opts |= QRegularExpression::CaseInsensitiveOption;
        m_keywordsRe = QRegularExpression(pattern, opts);
    }

    m_commentFormat.setForeground(m_definition->commentColor);
}

void RuleSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (!m_definition) {
        setCurrentBlockState(0);
        return;
    }

    int state = 0;
    const QTextBlock prev = currentBlock().previous();
    if (prev.isValid() && prev.userState() >= InComment)
        state = prev.userState();

    int pos = 0;

    // Continue an unclosed block comment from the previous block.
    if (state >= InComment) {
        const int pairIndex = state - InComment;
        if (pairIndex >= m_definition->blockComments.size()) {
            state = 0;
        } else {
            const QString &opener = m_definition->blockComments[pairIndex].first;
            const QString &closer = m_definition->blockComments[pairIndex].second;
            const int closeIdx = text.indexOf(closer, pos);
            if (closeIdx < 0) {
                setFormat(0, text.length(), m_commentFormat);
                setCurrentBlockState(state);
                return;
            }
            const int end = closeIdx + closer.length();
            setFormat(0, end, m_commentFormat);
            pos = end;
            state = 0;
        }
    }

    // Regex rules first (keywords, strings, numbers, functions ...); later
    // comment formats overwrite them where comments actually occur.
    {
        QTextCharFormat fmt;
        auto applyRule = [&text, this](const QRegularExpression &re, const QColor &color,
                                       bool bold, bool italic) {
            if (!re.isValid())
                return;
            auto it = re.globalMatch(text);
            while (it.hasNext()) {
                const QRegularExpressionMatch m = it.next();
                if (m.capturedLength() == 0)
                    continue;
                QTextCharFormat f;
                f.setForeground(color);
                if (bold)
                    f.setFontWeight(QFont::Bold);
                if (italic)
                    f.setFontItalic(true);
                setFormat(m.capturedStart(), m.capturedLength(), f);
            }
        };

        if (m_keywordsRe.isValid() && !m_definition->keywords.isEmpty()) {
            auto it = m_keywordsRe.globalMatch(text);
            while (it.hasNext()) {
                const QRegularExpressionMatch m = it.next();
                const QColor color = m_keywordColors.value(m.captured(),
                                                           Qt::darkBlue);
                QTextCharFormat f;
                f.setForeground(color);
                setFormat(m.capturedStart(), m.capturedLength(), f);
            }
        }
        for (const auto &rule : m_definition->regexRules)
            applyRule(rule.re, rule.color, rule.bold, rule.italic);
    }

    // Comments (highest priority): walk forward picking whichever construct
    // starts first — a line comment or a block comment opener.
    int newState = 0;
    bool done = false;
    while (!done) {
        int lcIdx = -1;
        for (const QString &lc : m_definition->lineComments) {
            const int idx = text.indexOf(lc, pos);
            if (idx >= 0 && (lcIdx < 0 || idx < lcIdx))
                lcIdx = idx;
        }

        int bcIdx = -1, bcPair = -1;
        for (int i = 0; i < m_definition->blockComments.size(); ++i) {
            const QString &opener = m_definition->blockComments[i].first;
            const int idx = text.indexOf(opener, pos);
            if (idx >= 0 && (bcIdx < 0 || idx < bcIdx)) {
                bcIdx = idx;
                bcPair = i;
            }
        }

        if (lcIdx < 0 && bcIdx < 0)
            break; // no more comments

        if (lcIdx >= 0 && (bcIdx < 0 || lcIdx <= bcIdx)) {
            setFormat(lcIdx, text.length() - lcIdx, m_commentFormat);
            break; // line comment runs to end of block
        }

        const QString &opener = m_definition->blockComments[bcPair].first;
        const QString &closer = m_definition->blockComments[bcPair].second;
        const int closeIdx = text.indexOf(closer, bcIdx + opener.length());
        if (closeIdx < 0) {
            setFormat(bcIdx, text.length() - bcIdx, m_commentFormat);
            newState = InComment + bcPair;
            done = true;
        } else {
            const int end = closeIdx + closer.length();
            setFormat(bcIdx, end - bcIdx, m_commentFormat);
            pos = end;
        }
    }

    setCurrentBlockState(newState);
}

} // namespace AppleCat::Core
