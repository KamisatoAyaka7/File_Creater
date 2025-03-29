#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFontComboBox>
#include <QSpinBox>
#include <QColorDialog>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 字体选择
    {
        QHBoxLayout *fontLayout = new QHBoxLayout;
        fontLayout->addWidget(new QLabel(tr("Font:")));
        fontComboBox = new QFontComboBox(this);
        fontLayout->addWidget(fontComboBox);
        layout->addLayout(fontLayout);
    }

    // 字体大小选择
    {
        QHBoxLayout *fontSizeLayout = new QHBoxLayout;
        fontSizeLayout->addWidget(new QLabel(tr("Font Size:")));
        fontSizeSpinBox = new QSpinBox(this);
        fontSizeSpinBox->setRange(8, 48);
        fontSizeSpinBox->setValue(12);
        fontSizeLayout->addWidget(fontSizeSpinBox);
        layout->addLayout(fontSizeLayout);
    }

    // 背景颜色选择
    {
        QHBoxLayout *colorLayout = new QHBoxLayout;
        colorLayout->addWidget(new QLabel(tr("Background Color:")));
        QPushButton *colorButton = new QPushButton(tr("Choose Color"), this);
        BGColorDialog = new QColorDialog(this);
        connect(colorButton, &QPushButton::clicked, BGColorDialog, &QColorDialog::exec);
        colorLayout->addWidget(colorButton);
        layout->addLayout(colorLayout);
    }

    // 行号绘制颜色选择
    {
        QHBoxLayout *colorLayout = new QHBoxLayout;
        colorLayout->addWidget(new QLabel(tr("NumberArea Color:")));
        QPushButton *colorButton = new QPushButton(tr("Choose Color"), this);
        lineNumberAreaColorDialog = new QColorDialog(this);
        connect(colorButton, &QPushButton::clicked, lineNumberAreaColorDialog, &QColorDialog::exec);
        colorLayout->addWidget(colorButton);
        layout->addLayout(colorLayout);
    }

    // 行号字体颜色选择
    {
        QHBoxLayout *colorLayout = new QHBoxLayout;
        colorLayout->addWidget(new QLabel(tr("LineNumber Color:")));
        QPushButton *colorButton = new QPushButton(tr("Choose Color"), this);
        blockNumberColorDialog = new QColorDialog(this);
        connect(colorButton, &QPushButton::clicked, blockNumberColorDialog, &QColorDialog::exec);
        colorLayout->addWidget(colorButton);
        layout->addLayout(colorLayout);
    }

    // 行颜色选择
    {
        QHBoxLayout *colorLayout = new QHBoxLayout;
        colorLayout->addWidget(new QLabel(tr("CurrentLine Color:")));
        QPushButton *colorButton = new QPushButton(tr("Choose Color"), this);
        lineColorDialog = new QColorDialog(this);
        connect(colorButton, &QPushButton::clicked, lineColorDialog, &QColorDialog::exec);
        colorLayout->addWidget(colorButton);
        layout->addLayout(colorLayout);
    }

    // 应用按钮
    QPushButton *applyButton = new QPushButton(tr("Apply"), this);
    connect(applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    layout->addWidget(applyButton);
}

QFont SettingsDialog::getSelectedFont() const
{
    QFont font = fontComboBox->currentFont();
    font.setPointSize(fontSizeSpinBox->value());
    return font;
}

QColor SettingsDialog::getSelectedBackgroundColor() const
{
    return BGColorDialog->currentColor();
}

QColor SettingsDialog::getSelectedLineNumberAreaColor() const
{
    return lineNumberAreaColorDialog->currentColor();
}

QColor SettingsDialog::getSelectedBlockNumberColor() const
{
    return blockNumberColorDialog->currentColor();
}

QColor SettingsDialog::getSelectedLineColor() const
{
    return lineColorDialog->currentColor();
}

void SettingsDialog::applySettings()
{
    emit settingsApplied(SettingsDialog::getSelectedFont(),
                         SettingsDialog::getSelectedBackgroundColor(),
                         SettingsDialog::getSelectedLineNumberAreaColor(),
                         SettingsDialog::getSelectedBlockNumberColor(),
                         SettingsDialog::getSelectedLineColor());
    accept();
}

void SettingsDialog::initColorDialogs(const QColor &backgroundColor,
                                      const QColor &lineNumberAreaColor,
                                      const QColor &blockNumberColor,
                                      const QColor &lineColor)
{
    BGColorDialog->setCurrentColor(backgroundColor);
    lineNumberAreaColorDialog->setCurrentColor(lineNumberAreaColor);
    blockNumberColorDialog->setCurrentColor(blockNumberColor);
    lineColorDialog->setCurrentColor(lineColor);
}
