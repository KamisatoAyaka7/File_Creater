#include "hexviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>

#define chunkSize 4096

HexViewer::HexViewer(QWidget *parent)
    : QWidget(parent), fileSize(0), currentPosition(0), currentChunk(0), totalChunks(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 显示格式选择
    QHBoxLayout *formatLayout = new QHBoxLayout;
    formatLayout->addWidget(new QLabel(tr("Display Format:")));
    formatComboBox = new QComboBox(this);
    formatComboBox->addItem("Hexadecimal");
    formatComboBox->addItem("Decimal");
    formatComboBox->addItem("Binary");
    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HexViewer::updateDisplayFormat);
    formatLayout->addWidget(formatComboBox);
    layout->addLayout(formatLayout);

    // 文本显示区域
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    QHBoxLayout *Hlayout = new QHBoxLayout;
    layout->addLayout(Hlayout);

    // 加载按钮
    loadButton = new QPushButton(tr("Load Chunk"), this);
    connect(loadButton, &QPushButton::clicked, this, &HexViewer::loadChunk);
    Hlayout->addWidget(loadButton);

    chunkEdit = new QLineEdit(this);
    connect(chunkEdit,&QLineEdit::returnPressed,this,&HexViewer::loadChunk);
    chunkEdit->setText("2");
    Hlayout->addWidget(chunkEdit);

    // 状态栏
    statusLabel = new QLabel(this);
    layout->addWidget(statusLabel);

    setLayout(layout);
}

void HexViewer::loadFile(const QString &fileName)
{
    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file"));
        return;
    }

    fileSize = file.size();
    currentPosition = 0;
    currentChunk = 0;
    totalChunks = (fileSize + 4095) / 4096; // 计算总 Chunk 数
    displayChunk();
}

void HexViewer::updateDisplayFormat(int index)
{
    Q_UNUSED(index);
    displayChunk(); // 切换进制时重新显示当前 Chunk
}

void HexViewer::loadChunk()
{
    qint64 Chunk=chunkEdit->text().toLongLong();
    if(Chunk>0)
    {
        currentChunk=Chunk-1;
        currentPosition=currentChunk*chunkSize;
        displayChunk();
        chunkEdit->setText(QString::number(Chunk+1));
    }
}

void HexViewer::displayChunk()
{
    if (currentPosition >= fileSize) {
        textEdit->append(tr("End of file reached."));
        return;
    }

    QByteArray data = file.read(chunkSize);

    QString displayText;
    switch (formatComboBox->currentIndex()) {
    case 0: // Hexadecimal
        displayText = data.toHex(' ');
        break;
    case 1: // Decimal
        for (char byte : data) {
            displayText += QString::number(static_cast<unsigned char>(byte), 10) + " ";
        }
        break;
    case 2: // Binary
        for (char byte : data) {
            displayText += QString::number(static_cast<unsigned char>(byte), 2) + " ";
        }
        break;
    }

    textEdit->setText(displayText);
    updateStatus(); // 更新状态栏
}

void HexViewer::updateStatus()
{
    statusLabel->setText(tr("Chunk: %1/%2").arg(currentChunk+1).arg(totalChunks));
}
