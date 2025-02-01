#include "codeeditor.h"
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextBlock>

QMap<QString, QColor> CodeEditor::globalKeywords;

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), m_completer(nullptr)
{
    // 初始化补全器
    QStringList words = globalKeywords.keys();
    QCompleter *completer = new QCompleter(words, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWidget(this);
    setCompleter(completer);
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
    if (currentLine.trimmed().endsWith('{')) {
        indentation += "    "; // 4 个空格
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

    // 处理 Tab 键触发补全
    if (e->key() == Qt::Key_Tab && m_completer) {
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
