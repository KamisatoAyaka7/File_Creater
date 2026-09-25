// Application settings: typed access, defaults, portable-mode INI and a
// change-notification signal used by the GUI to re-apply options live.

#pragma once

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QMap>

class QSettings;

namespace AppleCat::Core {

class AppSettings : public QObject
{
    Q_OBJECT

public:
    // Must be called once from main() before any other component touches
    // settings. Decides between portable mode (settings.ini next to the
    // executable, if such a file exists) and the per-user INI location.
    static void init();
    static AppSettings *instance();

    // Setting keys --------------------------------------------------------
    static constexpr const char *kEditorFontFamily      = "editor/fontFamily";
    static constexpr const char *kEditorFontSize        = "editor/fontSize";
    static constexpr const char *kTabWidth              = "editor/tabWidth";
    static constexpr const char *kAutoIndent            = "editor/autoIndent";
    static constexpr const char *kAutoCloseBrackets     = "editor/autoCloseBrackets";
    static constexpr const char *kWordWrap              = "editor/wordWrap";
    static constexpr const char *kShowLineNumbers       = "editor/showLineNumbers";
    static constexpr const char *kHighlightCurrentLine  = "editor/highlightCurrentLine";
    static constexpr const char *kColorEditorBg         = "colors/editorBackground";
    static constexpr const char *kColorLineNumBg        = "colors/lineNumberBackground";
    static constexpr const char *kColorLineNumFg        = "colors/lineNumberForeground";
    static constexpr const char *kColorCurrentLine      = "colors/currentLineHighlight";
    static constexpr const char *kColorFindHighlight    = "colors/findHighlight";
    static constexpr const char *kColorFindCurrent      = "colors/findHighlightCurrent";
    static constexpr const char *kCompletionEnabled     = "completion/enabled";
    static constexpr const char *kCompletionAutoPopup   = "completion/autoPopup";
    static constexpr const char *kCompletionMinPrefix   = "completion/minPrefix";
    static constexpr const char *kCompletionFuzzy       = "completion/fuzzyMatching";
    static constexpr const char *kCompletionKeywords    = "completion/keywords";
    static constexpr const char *kCompletionSnippets    = "completion/snippets";
    static constexpr const char *kCompletionDocWords    = "completion/documentWords";
    static constexpr const char *kEncodingDefaultOpen   = "encoding/defaultOpen";
    static constexpr const char *kEncodingDefaultSave   = "encoding/defaultSave";
    static constexpr const char *kEncodingFallback      = "encoding/fallback";
    static constexpr const char *kHexBytesPerRow        = "hex/bytesPerRow";
    static constexpr const char *kHexAddressRadix       = "hex/addressRadix";
    static constexpr const char *kHexShowAscii          = "hex/showAsciiColumn";
    static constexpr const char *kFindHighlightAll      = "find/highlightAll";
    static constexpr const char *kFindMaxHighlights     = "find/maxHighlights";
    static constexpr const char *kRecentFiles           = "files/recent";
    static constexpr const char *kPluginsDisabled       = "plugins/disabled";
    static constexpr const char *kWindowGeometry        = "window/geometry";

    // ---------------------------------------------------------------------

    QVariant value(const QString &key,
                   const QVariant &defaultOverride = QVariant()) const; // app default if missing
    void setValue(const QString &key, const QVariant &v);
    void setValues(const QMap<QString, QVariant> &batch); // emits once

    static QVariant defaultValue(const QString &key);

    bool isPortable() const { return m_portable; }
    QString settingsFilePath() const;

    // %APPDATA%/AppleCat (or the equivalent), used for user-provided
    // syntax definitions, snippets and plugins.
    QString userResourceDir() const;
    QStringList syntaxSearchPaths() const;
    QStringList snippetSearchPaths() const;
    QStringList pluginSearchPaths() const;

    // Recent files --------------------------------------------------------
    QStringList recentFiles() const;
    void pushRecentFile(const QString &path);

signals:
    void settingsChanged(const QStringList &keys);

private:
    AppSettings();

    QSettings *m_settings = nullptr;
    bool m_portable = false;
    QString m_portablePath;
};

} // namespace AppleCat::Core
