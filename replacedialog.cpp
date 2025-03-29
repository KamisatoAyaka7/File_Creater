#include "replacedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Replace"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 查找文本输入框
    QHBoxLayout *findLayout = new QHBoxLayout;
    findLayout->addWidget(new QLabel(tr("Find:")));
    findLineEdit = new QLineEdit(this);
    findLayout->addWidget(findLineEdit);
    layout->addLayout(findLayout);

    // 替换文本输入框
    QHBoxLayout *replaceLayout = new QHBoxLayout;
    replaceLayout->addWidget(new QLabel(tr("Replace with:")));
    replaceLineEdit = new QLineEdit(this);
    replaceLayout->addWidget(replaceLineEdit);
    layout->addLayout(replaceLayout);

    // 替换和取消按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    replaceButton = new QPushButton(tr("Replace"), this);
    cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // 连接按钮信号
    connect(replaceButton, &QPushButton::clicked, this, &ReplaceDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &ReplaceDialog::reject);
}

QString ReplaceDialog::getFindText() const
{
    return findLineEdit->text();
}

QString ReplaceDialog::getReplaceText() const
{
    return replaceLineEdit->text();
}
