#include "hexviewer.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

HexViewer::HexViewer(QWidget *parent) : QWidget(parent)
{
    textEdit = new QTextEdit;
    textEdit->setReadOnly(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textEdit);
    setLayout(layout);
}

void HexViewer::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file"));
        return;
    }

    QByteArray data = file.readAll();
    QString hexData, binaryData, octalData;

    for (char byte : data) {
        quint8 unsignedByte = static_cast<quint8>(byte);
        hexData.append(QString("%1 ").arg(unsignedByte, 2, 16, QLatin1Char('0')));
        binaryData.append(QString("%1 ").arg(unsignedByte, 8, 2, QLatin1Char('0')));
        octalData.append(QString("%1 ").arg(unsignedByte, 3, 8, QLatin1Char('0')));
    }

    textEdit->setPlainText("Hex: " + hexData + "\n\nBinary: " + binaryData + "\n\nOctal: " + octalData);
    file.close();
}
