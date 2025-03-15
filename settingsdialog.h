#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QFontComboBox;
class QSpinBox;
class QColorDialog;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr); // 构造函数接受 QWidget* 参数
    QFont getSelectedFont() const;
    QColor getSelectedBackgroundColor() const;

signals:
    void settingsApplied(const QFont &font, const QColor &backgroundColor);

private slots:
    void applySettings();

private:
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QColorDialog *colorDialog;
};

#endif // SETTINGSDIALOG_H
