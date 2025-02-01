#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFontComboBox>
#include <QSpinBox>
#include <QColorDialog>

SettingsDialog::SettingsDialog(const QFont &currentFont, const QColor &currentBackgroundColor, QWidget *parent)
    : QDialog(parent), currentBackgroundColor(currentBackgroundColor)
{
    setWindowTitle(tr("Settings"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 字体选择
    QHBoxLayout *fontLayout = new QHBoxLayout;
    fontLayout->addWidget(new QLabel(tr("Font:")));
    fontComboBox = new QFontComboBox(this);
    fontComboBox->setCurrentFont(currentFont);
    fontLayout->addWidget(fontComboBox);
    layout->addLayout(fontLayout);

    // 字体大小选择
    QHBoxLayout *fontSizeLayout = new QHBoxLayout;
    fontSizeLayout->addWidget(new QLabel(tr("Font Size:")));
    fontSizeSpinBox = new QSpinBox(this);
    fontSizeSpinBox->setRange(8, 48);
    fontSizeSpinBox->setValue(currentFont.pointSize());
    fontSizeLayout->addWidget(fontSizeSpinBox);
    layout->addLayout(fontSizeLayout);

    // 背景颜色选择
    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(new QLabel(tr("Background Color:")));
    QPushButton *colorButton = new QPushButton(tr("Choose Color"), this);
    colorDialog = new QColorDialog(this);
    colorDialog->setCurrentColor(currentBackgroundColor);
    connect(colorButton, &QPushButton::clicked, colorDialog, &QColorDialog::exec);
    colorLayout->addWidget(colorButton);
    layout->addLayout(colorLayout);

    // 应用按钮
    QPushButton *applyButton = new QPushButton(tr("Apply"), this);
    connect(applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    layout->addWidget(applyButton);
}

void SettingsDialog::applySettings()
{
    QFont font = fontComboBox->currentFont();
    font.setPointSize(fontSizeSpinBox->value());
    QColor backgroundColor = colorDialog->currentColor();

    emit settingsApplied(font, backgroundColor);
    accept();
}
