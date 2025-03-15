#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    QString getFindText() const;

signals:
    void findNext(); // 查找下一个信号

private slots:
    void onFindClicked();

private:
    QLineEdit *findLineEdit;
    QPushButton *findButton;
    QPushButton *cancelButton;
};

#endif // FINDDIALOG_H
