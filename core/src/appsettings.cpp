#include "appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace AppleCat::Core {

static AppSettings *g_instance = nullptr;

AppSettings *AppSettings::instance()
{
    return g_instance;
}

void AppSettings::init()
{
    if (g_instance)
        return;
    g_instance = new AppSettings();
}

AppSettings::AppSettings()
    : QObject(nullptr)
{
    const QString exeDir = QCoreApplication::applicationDirPath();
    m_portablePath = exeDir + QStringLiteral("/settings.ini");
    m_portable = QFileInfo::exists(m_portablePath);

    if (m_portable) {
        m_settings = new QSettings(m_portablePath, QSettings::IniFormat, this);
    } else {
        m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                   QStringLiteral("szy"), QStringLiteral("Apple_Cat"), this);
    }
    m_settings->setFallbacksEnabled(false);

    // Make sure the user resource location exists so users know where to
    // drop additional syntax files / snippets.
    QDir().mkpath(userResourceDir());
}

QVariant AppSettings::defaultValue(const QString &key)
{
    static const QMap<QString, QVariant> defaults = {
        {QLatin1String(kEditorFontFamily),     QStringLiteral("Consolas")},
        {QLatin1String(kEditorFontSize),       11},
        {QLatin1String(kTabWidth),             4},
        {QLatin1String(kAutoIndent),           true},
        {QLatin1String(kAutoCloseBrackets),    true},
        {QLatin1String(kWordWrap),             false},
        {QLatin1String(kShowLineNumbers),      true},
        {QLatin1String(kHighlightCurrentLine), true},
        {QLatin1String(kColorEditorBg),        QStringLiteral("#ffffff")},
        {QLatin1String(kColorLineNumBg),       QStringLiteral("#f2f2f2")},
        {QLatin1String(kColorLineNumFg),       QStringLiteral("#808080")},
        {QLatin1String(kColorCurrentLine),     QStringLiteral("#fffde7")},
        {QLatin1String(kColorFindHighlight),   QStringLiteral("#ffef9b")},
        {QLatin1String(kColorFindCurrent),     QStringLiteral("#ffd24d")},
        {QLatin1String(kCompletionEnabled),    true},
        {QLatin1String(kCompletionAutoPopup),  true},
        {QLatin1String(kCompletionMinPrefix),  2},
        {QLatin1String(kCompletionFuzzy),      true},
        {QLatin1String(kCompletionKeywords),   true},
        {QLatin1String(kCompletionSnippets),   true},
        {QLatin1String(kCompletionDocWords),   true},
        {QLatin1String(kEncodingDefaultOpen),  QStringLiteral("auto")},
        {QLatin1String(kEncodingDefaultSave),  QStringLiteral("UTF-8")},
        {QLatin1String(kEncodingFallback),     QStringLiteral("System")},
        {QLatin1String(kHexBytesPerRow),       16},
        {QLatin1String(kHexAddressRadix),      QStringLiteral("hex")},
        {QLatin1String(kHexShowAscii),         true},
        {QLatin1String(kFindHighlightAll),     true},
        {QLatin1String(kFindMaxHighlights),    2000},
        {QLatin1String(kRecentFiles),          QStringList()},
        {QLatin1String(kPluginsDisabled),      QStringList()},
        {QLatin1String(kWindowGeometry),       QByteArray()},
    };
    return defaults.value(key);
}

QVariant AppSettings::value(const QString &key, const QVariant &defaultOverride) const
{
    return m_settings->value(key,
                             defaultOverride.isValid() ? defaultOverride
                                                       : defaultValue(key));
}

void AppSettings::setValue(const QString &key, const QVariant &v)
{
    m_settings->setValue(key, v);
    emit settingsChanged({key});
}

void AppSettings::setValues(const QMap<QString, QVariant> &batch)
{
    QStringList keys = batch.keys();
    for (auto it = batch.begin(); it != batch.end(); ++it)
        m_settings->setValue(it.key(), it.value());
    m_settings->sync();
    emit settingsChanged(keys);
}

QString AppSettings::settingsFilePath() const
{
    return m_settings->fileName();
}

QString AppSettings::userResourceDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/AppleCat");
}

QStringList AppSettings::syntaxSearchPaths() const
{
    // Highest priority first; first definition with a given name wins.
    return {
        userResourceDir() + QStringLiteral("/syntax"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/syntax"),
        QStringLiteral(":/syntax"),
    };
}

QStringList AppSettings::snippetSearchPaths() const
{
    return {
        userResourceDir() + QStringLiteral("/snippets.json"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/snippets.json"),
        QStringLiteral(":/snippets.json"),
    };
}

QStringList AppSettings::pluginSearchPaths() const
{
    return {
        QCoreApplication::applicationDirPath() + QStringLiteral("/plugins"),
        userResourceDir() + QStringLiteral("/plugins"),
    };
}

QStringList AppSettings::recentFiles() const
{
    return value(kRecentFiles).toStringList();
}

void AppSettings::pushRecentFile(const QString &path)
{
    QStringList files = recentFiles();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > 10)
        files.removeLast();
    setValue(kRecentFiles, files);
}

} // namespace AppleCat::Core
