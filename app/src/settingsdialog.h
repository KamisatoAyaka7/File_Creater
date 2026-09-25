// Tabbed settings dialog: editor, colors, completion, encodings, hex view.
// "Apply" writes everything through AppSettings::setValues (one change
// signal); all open documents re-apply live.

#pragma once

#include <QColor>
#include <QDialog>

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QGridLayout;
class QGroupBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;

namespace AppleCat::Gui {

class PluginHost;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(PluginHost *pluginHost = nullptr, QWidget *parent = nullptr);

private slots:
    void apply();
    void resetDefaults();
    void rebuildPluginPage();

private:
    void buildEditorPage();
    void buildColorsPage();
    void buildCompletionPage();
    void buildEncodingPage();
    void buildHexPage();

    QWidget *page(const QString &title, QGridLayout *grid) const;

    PluginHost *m_pluginHost = nullptr;
    QWidget *m_pluginPage = nullptr;
    QVBoxLayout *m_pluginLayout = nullptr;

    // editor
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSize = nullptr;
    QSpinBox *m_tabWidth = nullptr;
    QCheckBox *m_autoIndent = nullptr;
    QCheckBox *m_autoClose = nullptr;
    QCheckBox *m_wordWrap = nullptr;
    QCheckBox *m_lineNumbers = nullptr;
    QCheckBox *m_highlightLine = nullptr;

    // colors
    struct ColorRow
    {
        QString key;
        QPushButton *button = nullptr;
        QColor color;
    };
    QList<ColorRow> m_colorRows;
    QPushButton *colorButton(const QString &key, QGridLayout *grid, int row);

    // completion
    QCheckBox *m_completionEnabled = nullptr;
    QCheckBox *m_autoPopup = nullptr;
    QCheckBox *m_fuzzy = nullptr;
    QSpinBox *m_minPrefix = nullptr;
    QCheckBox *m_kwSource = nullptr;
    QCheckBox *m_snipSource = nullptr;
    QCheckBox *m_docSource = nullptr;

    // encoding
    QComboBox *m_defaultOpen = nullptr;
    QComboBox *m_defaultSave = nullptr;
    QComboBox *m_fallback = nullptr;
    QCheckBox *m_bomCheck = nullptr; // per-save BOM lives on the tab; here: default

    // hex
    QSpinBox *m_hexBpr = nullptr;
    QComboBox *m_hexRadix = nullptr;
    QCheckBox *m_hexAscii = nullptr;

    // find
    QCheckBox *m_findHighlightAll = nullptr;
    QSpinBox *m_findMax = nullptr;

    QGridLayout *m_editorGrid = nullptr;
    QGridLayout *m_colorsGrid = nullptr;
};

} // namespace AppleCat::Gui
