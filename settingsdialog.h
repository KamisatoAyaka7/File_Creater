#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFont>
#include <QColor>

class QFontComboBox;
class QSpinBox;
class QColorDialog;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const QFont &currentFont, const QColor &currentBackgroundColor, QWidget *parent = nullptr);

signals:
    void settingsApplied(const QFont &font, const QColor &backgroundColor);

private slots:
    void applySettings();

private:
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QColorDialog *colorDialog;
    QColor currentBackgroundColor;
};

#endif // SETTINGSDIALOG_H
