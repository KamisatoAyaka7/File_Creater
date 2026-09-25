#include "settingsdialog.h"

#include "appsettings.h"
#include "encodingservice.h"
#include "pluginhost.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace AppleCat::Gui {

using AppleCat::Core::AppSettings;
using AppleCat::Core::EncodingService;

SettingsDialog::SettingsDialog(PluginHost *pluginHost, QWidget *parent)
    : QDialog(parent), m_pluginHost(pluginHost)
{
    setWindowTitle(tr("Settings"));
    resize(560, 460);

    auto *root = new QVBoxLayout(this);

    auto *body = new QHBoxLayout;
    auto *nav = new QListWidget(this);
    nav->setFixedWidth(140);
    auto *stack = new QStackedWidget(this);
    body->addWidget(nav);
    body->addWidget(stack, 1);
    root->addLayout(body, 1);

    auto addPage = [&](const QString &name, QWidget *page) {
        nav->addItem(name);
        stack->addWidget(page);
    };

    // Editor page
    {
        auto *grid = new QGridLayout;
        int row = 0;
        m_fontCombo = new QFontComboBox(this);
        grid->addWidget(new QLabel(tr("Font:"), this), row, 0);
        grid->addWidget(m_fontCombo, row, 1);
        ++row;
        m_fontSize = new QSpinBox(this);
        m_fontSize->setRange(6, 72);
        grid->addWidget(new QLabel(tr("Font size:"), this), row, 0);
        grid->addWidget(m_fontSize, row, 1);
        ++row;
        m_tabWidth = new QSpinBox(this);
        m_tabWidth->setRange(1, 16);
        grid->addWidget(new QLabel(tr("Tab width (spaces):"), this), row, 0);
        grid->addWidget(m_tabWidth, row, 1);
        ++row;
        m_autoIndent = new QCheckBox(tr("Auto indent on Enter"), this);
        grid->addWidget(m_autoIndent, row, 0, 1, 2);
        ++row;
        m_autoClose = new QCheckBox(tr("Auto-close brackets and quotes"), this);
        grid->addWidget(m_autoClose, row, 0, 1, 2);
        ++row;
        m_wordWrap = new QCheckBox(tr("Word wrap"), this);
        grid->addWidget(m_wordWrap, row, 0, 1, 2);
        ++row;
        m_lineNumbers = new QCheckBox(tr("Show line numbers"), this);
        grid->addWidget(m_lineNumbers, row, 0, 1, 2);
        ++row;
        m_highlightLine = new QCheckBox(tr("Highlight current line"), this);
        grid->addWidget(m_highlightLine, row, 0, 1, 2);
        ++row;
        addPage(tr("Editor"), page(tr("Editor"), grid));
    }

    // Colors page
    {
        auto *grid = new QGridLayout;
        int row = 0;
        struct Entry
        {
            const char *key;
            QString label;
        };
        const QList<Entry> entries = {
            {AppSettings::kColorEditorBg, tr("Editor background")},
            {AppSettings::kColorLineNumBg, tr("Line-number area")},
            {AppSettings::kColorLineNumFg, tr("Line numbers")},
            {AppSettings::kColorCurrentLine, tr("Current line")},
            {AppSettings::kColorFindHighlight, tr("Find highlight (all)")},
            {AppSettings::kColorFindCurrent, tr("Find highlight (current)")},
        };
        for (const Entry &e : entries) {
            grid->addWidget(new QLabel(e.label + QStringLiteral(":"), this), row, 0);
            grid->addWidget(colorButton(e.key, grid, row), row, 1);
            ++row;
        }
        addPage(tr("Colors"), page(tr("Colors"), grid));
    }

    // Completion page
    {
        auto *grid = new QGridLayout;
        int row = 0;
        m_completionEnabled = new QCheckBox(tr("Enable completion"), this);
        grid->addWidget(m_completionEnabled, row, 0, 1, 2);
        ++row;
        m_autoPopup = new QCheckBox(tr("Popup automatically while typing"), this);
        grid->addWidget(m_autoPopup, row, 0, 1, 2);
        ++row;
        m_fuzzy = new QCheckBox(tr("Fuzzy (subsequence) matching"), this);
        grid->addWidget(m_fuzzy, row, 0, 1, 2);
        ++row;
        m_minPrefix = new QSpinBox(this);
        m_minPrefix->setRange(1, 6);
        grid->addWidget(new QLabel(tr("Minimum prefix length:"), this), row, 0);
        grid->addWidget(m_minPrefix, row, 1);
        ++row;
        auto *sources = new QGroupBox(tr("Sources"), this);
        auto *srcGrid = new QGridLayout(sources);
        m_kwSource = new QCheckBox(tr("Language keywords"), sources);
        m_snipSource = new QCheckBox(tr("Snippets"), sources);
        m_docSource = new QCheckBox(tr("Words from this document"), sources);
        srcGrid->addWidget(m_kwSource, 0, 0);
        srcGrid->addWidget(m_snipSource, 1, 0);
        srcGrid->addWidget(m_docSource, 2, 0);
        grid->addWidget(sources, row, 0, 1, 2);
        addPage(tr("Completion"), page(tr("Completion"), grid));
    }

    // Encoding page
    {
        auto *grid = new QGridLayout;
        int row = 0;
        auto fill = [&](QComboBox *box, bool withAuto) {
            if (withAuto)
                box->addItem(EncodingService::AUTO);
            for (const auto &info : EncodingService::instance().encodings())
                box->addItem(info.id);
        };
        m_defaultOpen = new QComboBox(this);
        fill(m_defaultOpen, true);
        grid->addWidget(new QLabel(tr("Encoding when opening:"), this), row, 0);
        grid->addWidget(m_defaultOpen, row, 1);
        ++row;
        m_defaultSave = new QComboBox(this);
        fill(m_defaultSave, false);
        grid->addWidget(new QLabel(tr("Encoding when saving:"), this), row, 0);
        grid->addWidget(m_defaultSave, row, 1);
        ++row;
        m_fallback = new QComboBox(this);
        fill(m_fallback, false);
        grid->addWidget(new QLabel(tr("Detection fallback (legacy):"), this), row, 0);
        grid->addWidget(m_fallback, row, 1);
        ++row;
        auto *hint = new QLabel(
            tr("Also available per tab from the encoding combo in the toolbar.\n"
               "A BOM is written for UTF-8/16/32 when the tab's BOM toggle is on."),
            this);
        hint->setWordWrap(true);
        grid->addWidget(hint, row, 0, 1, 2);
        addPage(tr("Encoding"), page(tr("Encoding"), grid));
    }

    // Hex + find page
    {
        auto *grid = new QGridLayout;
        int row = 0;
        m_hexBpr = new QSpinBox(this);
        m_hexBpr->setRange(8, 64);
        m_hexBpr->setSingleStep(8);
        grid->addWidget(new QLabel(tr("Hex editor bytes per row:"), this), row, 0);
        grid->addWidget(m_hexBpr, row, 1);
        ++row;
        m_hexAscii = new QCheckBox(tr("Show ASCII column"), this);
        grid->addWidget(m_hexAscii, row, 0, 1, 2);
        ++row;
        grid->addWidget(new QLabel(QStringLiteral("")), row, 0);
        ++row;
        m_findHighlightAll = new QCheckBox(tr("Highlight all find matches"), this);
        grid->addWidget(m_findHighlightAll, row, 0, 1, 2);
        ++row;
        m_findMax = new QSpinBox(this);
        m_findMax->setRange(100, 200000);
        grid->addWidget(new QLabel(tr("Max find highlights:"), this), row, 0);
        grid->addWidget(m_findMax, row, 1);
        ++row;
        addPage(tr("Hex & Find"), page(tr("Hex & Find"), grid));
    }

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Apply
                                         | QDialogButtonBox::Cancel
                                         | QDialogButtonBox::RestoreDefaults,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        apply();
        accept();
    });
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsDialog::resetDefaults);
    root->addWidget(buttons);

    connect(nav, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);
    nav->setCurrentRow(0);

    // Plugins page (built from the live plugin host) -----------------------
    if (m_pluginHost) {
        m_pluginPage = new QWidget;
        m_pluginLayout = new QVBoxLayout(m_pluginPage);
        rebuildPluginPage();
        m_pluginLayout->addStretch(1);
        auto *hint = new QLabel(
            tr("Plugins are loaded from the \"plugins\" folder next to the "
               "executable (or %APPDATA%/AppleCat/plugins). Changes take "
               "effect immediately; newly enabled plugins load at once."),
            m_pluginPage);
        hint->setWordWrap(true);
        m_pluginLayout->addWidget(hint);
        addPage(tr("Plugins"), m_pluginPage);
    }

    // Load current values ------------------------------------------------
    auto *s = AppSettings::instance();
    m_fontCombo->setCurrentText(s->value(AppSettings::kEditorFontFamily).toString());
    m_fontSize->setValue(s->value(AppSettings::kEditorFontSize).toInt());
    m_tabWidth->setValue(s->value(AppSettings::kTabWidth).toInt());
    m_autoIndent->setChecked(s->value(AppSettings::kAutoIndent).toBool());
    m_autoClose->setChecked(s->value(AppSettings::kAutoCloseBrackets).toBool());
    m_wordWrap->setChecked(s->value(AppSettings::kWordWrap).toBool());
    m_lineNumbers->setChecked(s->value(AppSettings::kShowLineNumbers).toBool());
    m_highlightLine->setChecked(s->value(AppSettings::kHighlightCurrentLine).toBool());

    m_completionEnabled->setChecked(s->value(AppSettings::kCompletionEnabled).toBool());
    m_autoPopup->setChecked(s->value(AppSettings::kCompletionAutoPopup).toBool());
    m_fuzzy->setChecked(s->value(AppSettings::kCompletionFuzzy).toBool());
    m_minPrefix->setValue(s->value(AppSettings::kCompletionMinPrefix).toInt());
    m_kwSource->setChecked(s->value(AppSettings::kCompletionKeywords).toBool());
    m_snipSource->setChecked(s->value(AppSettings::kCompletionSnippets).toBool());
    m_docSource->setChecked(s->value(AppSettings::kCompletionDocWords).toBool());

    const QString openEnc = s->value(AppSettings::kEncodingDefaultOpen).toString();
    m_defaultOpen->setCurrentText(openEnc);
    m_defaultSave->setCurrentText(s->value(AppSettings::kEncodingDefaultSave).toString());
    m_fallback->setCurrentText(s->value(AppSettings::kEncodingFallback).toString());

    m_hexBpr->setValue(s->value(AppSettings::kHexBytesPerRow).toInt());
    m_hexAscii->setChecked(s->value(AppSettings::kHexShowAscii).toBool());
    m_findHighlightAll->setChecked(s->value(AppSettings::kFindHighlightAll).toBool());
    m_findMax->setValue(s->value(AppSettings::kFindMaxHighlights).toInt());
}

QWidget *SettingsDialog::page(const QString &, QGridLayout *grid) const
{
    auto *w = new QWidget;
    auto *layout = new QVBoxLayout(w);
    layout->addLayout(grid);
    layout->addStretch(1);
    return w;
}

QPushButton *SettingsDialog::colorButton(const QString &key, QGridLayout *, int row)
{
    Q_UNUSED(row);
    ColorRow cr;
    cr.key = key;
    cr.color = QColor(AppSettings::instance()->value(key).toString());
    cr.button = new QPushButton(cr.color.name().toUpper(), this);
    cr.button->setAutoFillBackground(false);

    const QString keyCopy = key;
    connect(cr.button, &QPushButton::clicked, this, [this, keyCopy]() {
        for (ColorRow &r : m_colorRows) {
            if (r.key != keyCopy)
                continue;
            const QColor chosen = QColorDialog::getColor(r.color, this, tr("Choose color"));
            if (chosen.isValid()) {
                r.color = chosen;
                r.button->setText(chosen.name().toUpper());
                r.button->setStyleSheet(
                    QStringLiteral("background: %1").arg(chosen.name()));
            }
            break;
        }
    });
    cr.button->setStyleSheet(QStringLiteral("background: %1").arg(cr.color.name()));
    m_colorRows.append(cr);
    return cr.button;
}

void SettingsDialog::apply()
{
    auto *s = AppSettings::instance();
    QMap<QString, QVariant> batch;
    batch.insert(QLatin1String(AppSettings::kEditorFontFamily), m_fontCombo->currentText());
    batch.insert(QLatin1String(AppSettings::kEditorFontSize), m_fontSize->value());
    batch.insert(QLatin1String(AppSettings::kTabWidth), m_tabWidth->value());
    batch.insert(QLatin1String(AppSettings::kAutoIndent), m_autoIndent->isChecked());
    batch.insert(QLatin1String(AppSettings::kAutoCloseBrackets), m_autoClose->isChecked());
    batch.insert(QLatin1String(AppSettings::kWordWrap), m_wordWrap->isChecked());
    batch.insert(QLatin1String(AppSettings::kShowLineNumbers), m_lineNumbers->isChecked());
    batch.insert(QLatin1String(AppSettings::kHighlightCurrentLine), m_highlightLine->isChecked());

    for (const ColorRow &r : m_colorRows)
        batch.insert(r.key, r.color.name());

    batch.insert(QLatin1String(AppSettings::kCompletionEnabled), m_completionEnabled->isChecked());
    batch.insert(QLatin1String(AppSettings::kCompletionAutoPopup), m_autoPopup->isChecked());
    batch.insert(QLatin1String(AppSettings::kCompletionFuzzy), m_fuzzy->isChecked());
    batch.insert(QLatin1String(AppSettings::kCompletionMinPrefix), m_minPrefix->value());
    batch.insert(QLatin1String(AppSettings::kCompletionKeywords), m_kwSource->isChecked());
    batch.insert(QLatin1String(AppSettings::kCompletionSnippets), m_snipSource->isChecked());
    batch.insert(QLatin1String(AppSettings::kCompletionDocWords), m_docSource->isChecked());

    batch.insert(QLatin1String(AppSettings::kEncodingDefaultOpen), m_defaultOpen->currentText());
    batch.insert(QLatin1String(AppSettings::kEncodingDefaultSave), m_defaultSave->currentText());
    batch.insert(QLatin1String(AppSettings::kEncodingFallback), m_fallback->currentText());

    batch.insert(QLatin1String(AppSettings::kHexBytesPerRow), m_hexBpr->value());
    batch.insert(QLatin1String(AppSettings::kHexShowAscii), m_hexAscii->isChecked());
    batch.insert(QLatin1String(AppSettings::kFindHighlightAll), m_findHighlightAll->isChecked());
    batch.insert(QLatin1String(AppSettings::kFindMaxHighlights), m_findMax->value());

    s->setValues(batch);
    emit accepted();
}

void SettingsDialog::resetDefaults()
{    m_fontCombo->setCurrentText(AppSettings::defaultValue(AppSettings::kEditorFontFamily).toString());
    m_fontSize->setValue(AppSettings::defaultValue(AppSettings::kEditorFontSize).toInt());
    m_tabWidth->setValue(AppSettings::defaultValue(AppSettings::kTabWidth).toInt());
    m_autoIndent->setChecked(AppSettings::defaultValue(AppSettings::kAutoIndent).toBool());
    m_autoClose->setChecked(AppSettings::defaultValue(AppSettings::kAutoCloseBrackets).toBool());
    m_wordWrap->setChecked(AppSettings::defaultValue(AppSettings::kWordWrap).toBool());
    m_lineNumbers->setChecked(AppSettings::defaultValue(AppSettings::kShowLineNumbers).toBool());
    m_highlightLine->setChecked(AppSettings::defaultValue(AppSettings::kHighlightCurrentLine).toBool());

    for (ColorRow &r : m_colorRows) {
        r.color = QColor(AppSettings::defaultValue(r.key).toString());
        r.button->setText(r.color.name().toUpper());
        r.button->setStyleSheet(QStringLiteral("background: %1").arg(r.color.name()));
    }

    m_completionEnabled->setChecked(AppSettings::defaultValue(AppSettings::kCompletionEnabled).toBool());
    m_autoPopup->setChecked(AppSettings::defaultValue(AppSettings::kCompletionAutoPopup).toBool());
    m_fuzzy->setChecked(AppSettings::defaultValue(AppSettings::kCompletionFuzzy).toBool());
    m_minPrefix->setValue(AppSettings::defaultValue(AppSettings::kCompletionMinPrefix).toInt());
    m_kwSource->setChecked(AppSettings::defaultValue(AppSettings::kCompletionKeywords).toBool());
    m_snipSource->setChecked(AppSettings::defaultValue(AppSettings::kCompletionSnippets).toBool());
    m_docSource->setChecked(AppSettings::defaultValue(AppSettings::kCompletionDocWords).toBool());

    m_defaultOpen->setCurrentText(AppSettings::defaultValue(AppSettings::kEncodingDefaultOpen).toString());
    m_defaultSave->setCurrentText(AppSettings::defaultValue(AppSettings::kEncodingDefaultSave).toString());
    m_fallback->setCurrentText(AppSettings::defaultValue(AppSettings::kEncodingFallback).toString());

    m_hexBpr->setValue(AppSettings::defaultValue(AppSettings::kHexBytesPerRow).toInt());
    m_hexAscii->setChecked(AppSettings::defaultValue(AppSettings::kHexShowAscii).toBool());
    m_findHighlightAll->setChecked(AppSettings::defaultValue(AppSettings::kFindHighlightAll).toBool());
    m_findMax->setValue(AppSettings::defaultValue(AppSettings::kFindMaxHighlights).toInt());
}

void SettingsDialog::rebuildPluginPage()
{
    if (!m_pluginLayout || !m_pluginHost)
        return;
    // Clear previous rows.
    while (QLayoutItem *item = m_pluginLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    const auto plugins = m_pluginHost->plugins();
    if (plugins.isEmpty()) {
        m_pluginLayout->addWidget(new QLabel(tr("No plugins found."), m_pluginPage));
        return;
    }

    for (const auto &p : plugins) {
        auto *box = new QGroupBox(p.name, m_pluginPage);
        auto *grid = new QGridLayout(box);

        auto *enableCheck = new QCheckBox(tr("Enabled"), box);
        enableCheck->setChecked(p.enabled && p.active);
        const QString name = p.name;
        connect(enableCheck, &QCheckBox::toggled, box, [this, name](bool on) {
            m_pluginHost->setEnabled(name, on);
            rebuildPluginPage();
        });
        grid->addWidget(enableCheck, 0, 0);

        QString info = QStringLiteral("v%1  ·  %2").arg(p.version, p.vendor);
        if (!p.description.isEmpty())
            info += QStringLiteral("\n") + p.description;
        if (!p.error.isEmpty())
            info += QStringLiteral("\n⚠ ") + p.error;
        auto *infoLabel = new QLabel(info, box);
        infoLabel->setWordWrap(true);
        grid->addWidget(infoLabel, 1, 0);

        m_pluginLayout->addWidget(box);
    }
}

} // namespace AppleCat::Gui
