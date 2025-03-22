#include "finddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Find"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 查找文本输入框
    QHBoxLayout *findLayout = new QHBoxLayout;
    findLayout->addWidget(new QLabel(tr("Find:")));
    findLineEdit = new QLineEdit(this);
    findLayout->addWidget(findLineEdit);
    layout->addLayout(findLayout);

    // 查找、查找下一个和取消按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    findButton = new QPushButton(tr("Find"), this);
    cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(findButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // 连接按钮信号
    connect(findButton, &QPushButton::clicked, this, &FindDialog::onFindClicked);
    connect(cancelButton, &QPushButton::clicked, this, &FindDialog::reject);
}

QString FindDialog::getFindText() const
{
    return findLineEdit->text();
}

void FindDialog::onFindClicked()
{
    emit findNext(); // 发射查找信号
}

