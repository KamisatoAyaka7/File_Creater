// Rule-driven syntax highlighter. Keywords (with per-word colors, combined
// into a single regex for speed), line/block comments (with cross-block
// state tracking for multi-line comments and doc strings), and arbitrary
// regex rules (strings, numbers, functions, ...).

#pragma once

#include "syntaxdefinition.h"

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

#include <memory>

namespace AppleCat::Core {

class RuleSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    RuleSyntaxHighlighter(QTextDocument *document,
                          std::shared_ptr<const SyntaxDefinition> definition);
    ~RuleSyntaxHighlighter() override;

    void setDefinition(std::shared_ptr<const SyntaxDefinition> definition);

protected:
    void highlightBlock(const QString &text) override;

private:
    void rebuild();

    std::shared_ptr<const SyntaxDefinition> m_definition;
    QRegularExpression m_keywordsRe; // combined (?<!\w)(?:a|b|c)(?!\w)
    QHash<QString, QColor> m_keywordColors;
    QTextCharFormat m_commentFormat;
};

} // namespace AppleCat::Core
