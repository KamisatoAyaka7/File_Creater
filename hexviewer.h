#ifndef HEXVIEWER_H
#define HEXVIEWER_H

#include <QWidget>
#include <QFile>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

class HexViewer : public QWidget
{
    Q_OBJECT

public:
    explicit HexViewer(QWidget *parent = nullptr);
    void loadFile(const QString &fileName);

private slots:
    void updateDisplayFormat(int index);
    void loadChunk();

private:
    QTextEdit *textEdit;
    QComboBox *formatComboBox;
    QPushButton *loadButton;
    QLabel *statusLabel; // 新增：状态栏标签
    QLineEdit *chunkEdit;
    QFile file;
    qint64 fileSize;
    qint64 currentPosition;
    int currentChunk; // 当前 Chunk 序数
    int totalChunks;  // 总 Chunk 数

    void displayChunk();
    void updateStatus(); // 新增：更新状态栏
};

#endif // HEXVIEWER_H
