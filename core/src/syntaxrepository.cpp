#include "syntaxrepository.h"

#include "appsettings.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace AppleCat::Core {

static SyntaxRepository *g_instance = nullptr;

SyntaxRepository &SyntaxRepository::instance()
{
    if (!g_instance) {
        g_instance = new SyntaxRepository();
        g_instance->init();
    }
    return *g_instance;
}

void SyntaxRepository::init()
{
    m_all.clear();
    m_byExt.clear();
    m_loadedNames.clear();

    const QStringList paths = AppSettings::instance()
                                  ? AppSettings::instance()->syntaxSearchPaths()
                                  : QStringList{QStringLiteral(":/syntax")};
    // Highest priority first; embedded resources are last.
    for (const QString &path : paths) {
        if (path.startsWith(QLatin1Char(':')))
            loadFromDir(path);
        else if (QFileInfo::exists(path))
            loadFromDir(path);
    }
}

void SyntaxRepository::loadFromDir(const QString &dir)
{
    const QDir d(dir);
    const QStringList files = d.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QString &file : files)
        loadFromFile(d.filePath(file));
}

void SyntaxRepository::loadFromFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();

    auto def = std::make_shared<SyntaxDefinition>();
    def->name = root.value(QStringLiteral("name")).toString();
    if (def->name.isEmpty())
        def->name = QFileInfo(path).completeBaseName();

    if (m_loadedNames.contains(def->name))
        return; // a higher-priority file already defined this language

    const QJsonArray exts = root.value(QStringLiteral("extensions")).toArray();
    for (const QJsonValue &v : exts)
        def->extensions.append(v.toString().toLower());

    def->caseSensitiveKeywords = root.value(QStringLiteral("caseSensitive")).toBool(false);

    const QJsonArray kws = root.value(QStringLiteral("keywords")).toArray();
    for (const QJsonValue &v : kws) {
        const QJsonObject o = v.toObject();
        const QString word = o.value(QStringLiteral("word")).toString();
        if (word.isEmpty())
            continue;
        QColor color(o.value(QStringLiteral("color")).toString());
        if (!color.isValid())
            color = Qt::darkBlue;
        def->keywords.append({word, color});
    }

    if (root.contains(QStringLiteral("lineComment"))) {
        const QJsonValue lc = root.value(QStringLiteral("lineComment"));
        if (lc.isArray()) {
            for (const QJsonValue &v : lc.toArray())
                def->lineComments.append(v.toString());
        } else if (lc.isString()) {
            def->lineComments.append(lc.toString());
        }
    }

    const QJsonArray bcs = root.value(QStringLiteral("blockComments")).toArray();
    for (const QJsonValue &v : bcs) {
        if (v.isArray() && v.toArray().size() == 2) {
            const QString open = v.toArray().at(0).toString();
            const QString close = v.toArray().at(1).toString();
            if (!open.isEmpty() && !close.isEmpty())
                def->blockComments.append({open, close});
        }
    }

    if (root.contains(QStringLiteral("commentColor"))) {
        QColor c(root.value(QStringLiteral("commentColor")).toString());
        if (c.isValid())
            def->commentColor = c;
    }

    const QJsonArray rules = root.value(QStringLiteral("regexRules")).toArray();
    for (const QJsonValue &v : rules) {
        const QJsonObject o = v.toObject();
        const QString pattern = o.value(QStringLiteral("pattern")).toString();
        if (pattern.isEmpty())
            continue;
        SyntaxDefinition::RegexRule rule;
        rule.re = QRegularExpression(pattern);
        if (!rule.re.isValid())
            continue;
        QColor c(o.value(QStringLiteral("color")).toString());
        if (!c.isValid())
            c = Qt::darkRed;
        rule.color = c;
        rule.bold = o.value(QStringLiteral("bold")).toBool(false);
        rule.italic = o.value(QStringLiteral("italic")).toBool(false);
        def->regexRules.append(rule);
    }

    m_loadedNames.insert(def->name);
    m_all.append(def);
    index(def);
}

void SyntaxRepository::index(std::shared_ptr<const SyntaxDefinition> def)
{
    for (const QString &ext : def->extensions) {
        if (!m_byExt.contains(ext))
            m_byExt.insert(ext, def);
    }
}

std::shared_ptr<const SyntaxDefinition>
SyntaxRepository::definitionForExtension(const QString &extension) const
{
    return m_byExt.value(extension.toLower());
}

std::shared_ptr<const SyntaxDefinition>
SyntaxRepository::definitionForFile(const QString &filePath) const
{
    const QString suffix = QFileInfo(filePath).suffix();
    if (!suffix.isEmpty())
        return definitionForExtension(suffix);
    return definitionForExtension(QFileInfo(filePath).fileName());
}

} // namespace AppleCat::Core
