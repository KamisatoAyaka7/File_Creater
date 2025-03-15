#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QCompleter>
#include <QMap>
#include <QColor>
#include <QWidget>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void setCompleter(QCompleter *completer);
    QCompleter *completer() const;

    void lineNumberAreaPaintEvent(QPaintEvent *event); // 绘制行号区域
    int lineNumberAreaWidth(); // 计算行号区域宽度

    static void setGlobalKeywords(const QMap<QString, QColor> &keywords);

    void setFileName(QString name);
    QString toFileName();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void resizeEvent(QResizeEvent *event) override; // 重写 resizeEvent

private slots:
    void insertCompletion(const QString &completion);
    void updateLineNumberAreaWidth(int newBlockCount); // 更新行号区域宽度
    void highlightCurrentLine(); // 高亮当前行
    void updateLineNumberArea(const QRect &rect, int dy); // 更新行号区域

private:
    QString textUnderCursor() const;
    void handleIndentation();

    QCompleter *m_completer;
    static QMap<QString, QColor> globalKeywords;

    QWidget *lineNumberArea; // 行号区域

    QString fileName="Untitled";
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

#endif // CODEEDITOR_H
