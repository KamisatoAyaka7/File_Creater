// A syntax definition is loaded from a JSON file and drives both the
// highlighter and the keyword completion source.
//
// Format (all fields optional except keywords):
// {
//   "name": "C++",
//   "extensions": ["cpp", "cc", "h"],
//   "caseSensitive": false,
//   "keywords": [ {"word": "class", "color": "#155bFF"}, ... ],
//   "lineComment": "//",            // or an array, e.g. [";", "#"]
//   "blockComments": [["/*", "*/"]],
//   "commentColor": "#008000",
//   "regexRules": [
//     {"pattern": "\"(?:[^\"\\\\]|\\\\.)*\"", "color": "#a31515"}
//   ]
// }

#pragma once

#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

#include <memory>

namespace AppleCat::Core {

struct SyntaxDefinition
{
    struct RegexRule
    {
        QRegularExpression re;
        QColor color;
        bool bold = false;
        bool italic = false;
    };

    QString name;
    QStringList extensions;
    bool caseSensitiveKeywords = false;

    // word -> color (also used as the completion keyword list)
    QList<QPair<QString, QColor>> keywords;

    QStringList lineComments;
    QVector<QPair<QString, QString>> blockComments; // (open, close)
    QColor commentColor = QColor(0x00, 0x80, 0x00);

    QVector<RegexRule> regexRules; // strings, numbers, functions, ...

    bool isNull() const { return name.isEmpty(); }
};

} // namespace AppleCat::Core
