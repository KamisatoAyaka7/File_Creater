#ifndef HEXVIEWER_H
#define HEXVIEWER_H

#include <QWidget>
#include <QTextEdit>
#include <QFile>
#include <QDataStream>

class HexViewer : public QWidget
{
    Q_OBJECT

public:
    HexViewer(QWidget *parent = nullptr);

    void loadFile(const QString &fileName);

private:
    QTextEdit *textEdit;
};

#endif // HEXVIEWER_H
