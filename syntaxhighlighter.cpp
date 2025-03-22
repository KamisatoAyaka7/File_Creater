#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent, const QMap<QString, QColor> &keywords)
    : QSyntaxHighlighter(parent)
{
    setupRules(keywords);
}

void SyntaxHighlighter::setupRules(const QMap<QString, QColor> &keywords)
{
    // 关键字高亮
    for (auto it = keywords.begin(); it != keywords.end(); ++it) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b" + it.key() + "\\b");
        rule.format.setForeground(it.value());
        highlightingRules.append(rule);
    }

    // 单行注释高亮
    HighlightingRule singleLineCommentRule;
    singleLineCommentRule.pattern = QRegularExpression("//[^\n]*");
    singleLineCommentRule.format.setForeground(Qt::darkGreen);
    highlightingRules.append(singleLineCommentRule);

    // 多行注释高亮
    HighlightingRule multiLineCommentRule;
    multiLineCommentRule.pattern = QRegularExpression("/\\*.*\\*/");
    multiLineCommentRule.format.setForeground(Qt::darkGreen);
    highlightingRules.append(multiLineCommentRule);

    // 字符串高亮
    HighlightingRule quotationRule;
    quotationRule.pattern = QRegularExpression("\".*\"");
    quotationRule.format.setForeground(Qt::darkRed);
    highlightingRules.append(quotationRule);

    // 函数高亮
    HighlightingRule functionRule;
    functionRule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    functionRule.format.setForeground(Qt::blue);
    highlightingRules.append(functionRule);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
