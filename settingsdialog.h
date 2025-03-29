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
    QColor getSelectedLineNumberAreaColor() const;
    QColor getSelectedBlockNumberColor() const;
    QColor getSelectedLineColor() const;

    void initColorDialogs(const QColor &backgroundColor,
                          const QColor &lineNumberAreaColor,
                          const QColor &blockNumberColor,
                          const QColor &lineColor);

signals:
    void settingsApplied(const QFont &font,
                         const QColor &backgroundColor,
                         const QColor &lineNumberAreaColor,
                         const QColor &blockNumberColor,
                         const QColor &lineColor);

private slots:
    void applySettings();

private:
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QColorDialog *BGColorDialog;
    QColorDialog *lineNumberAreaColorDialog;
    QColorDialog *blockNumberColorDialog;
    QColorDialog *lineColorDialog;
};

#endif // SETTINGSDIALOG_H
