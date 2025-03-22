#include "codeeditor.h"
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextBlock>
#include <QPainter>
#include <QThread>

QMap<QString, QColor> CodeEditor::globalKeywords;

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), m_completer(nullptr)
{
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // 初始化补全器
    QStringList words = globalKeywords.keys();
    QCompleter *completer = new QCompleter(words, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWidget(this);
    setCompleter(completer);

    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::setGlobalKeywords(const QMap<QString, QColor> &keywords)
{
    globalKeywords = keywords;
}
void CodeEditor::setCompleter(QCompleter *completer)
{
    if (m_completer)
        m_completer->disconnect(this);

    m_completer = completer;

    if (!m_completer)
        return;

    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &CodeEditor::insertCompletion);
}

QCompleter *CodeEditor::completer() const
{
    return m_completer;
}

void CodeEditor::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != this)
        return;

    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    tc.removeSelectedText();
    tc.insertText(completion);
    setTextCursor(tc);
}

QString CodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void CodeEditor::handleIndentation()
{
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    QString currentLine = block.text();

    // 获取当前行的缩进
    QString indentation;
    for (int i = 0; i < currentLine.length(); ++i) {
        if (currentLine.at(i).isSpace()) {
            indentation += currentLine.at(i);
        } else {
            break;
        }
    }

    // 如果当前行以 '{' 结尾，则下一行增加缩进
    if (currentLine.trimmed().endsWith('{')||currentLine.trimmed().endsWith(':')) {
        indentation += "    "; //4个空格
    }

    // 插入新行并补全缩进
    cursor.insertText("\n" + indentation);
    setTextCursor(cursor);
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    if (m_completer && m_completer->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default:
            break;
        }
    }

    // 处理 Ctrl Tab 键触发补全
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if ((modifiers & (Qt::ControlModifier)) == (Qt::ControlModifier) && (e->key() == Qt::Key_Tab) && m_completer) {
        QString completionPrefix = textUnderCursor();
        if (!completionPrefix.isEmpty()) {
            m_completer->setCompletionPrefix(completionPrefix);
            m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));

            QRect cr = cursorRect();
            cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                        + m_completer->popup()->verticalScrollBar()->sizeHint().width());
            m_completer->complete(cr);
            return;
        }
    }

    // 处理 Enter 键补全缩进
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        handleIndentation();
        return;
    }

    QPlainTextEdit::keyPressEvent(e);
}

void CodeEditor::focusInEvent(QFocusEvent *e)
{
    if (m_completer)
        m_completer->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray); // 行号区域背景色

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black); // 行号颜色
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
        ++blockNumber;
    }
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::updateLineNumberAreaWidth(int newBlockCount)
{
    Q_UNUSED(newBlockCount);
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(160); // 高亮当前行颜色
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::setFileName(QString name)
{
    CodeEditor::fileName=name;
}

QString CodeEditor::toFileName()
{
    return CodeEditor::fileName;
}
