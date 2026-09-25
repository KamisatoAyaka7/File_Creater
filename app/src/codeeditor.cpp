#include "codeeditor.h"

#include "appsettings.h"
#include "completion.h"
#include "completionpopup.h"
#include "syntaxhighlighter.h"
#include "syntaxrepository.h"

#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTimer>

namespace AppleCat::Gui {

namespace {
QColor kindColor(int kind)
{
    switch (kind) {
    case 0: return QColor(0x04, 0x51, 0xa5); // keyword
    case 1: return QColor(0x00, 0x80, 0x00); // snippet
    case 3: return QColor(0x80, 0x19, 0xFF); // plugin
    default: return QColor(0x60, 0x60, 0x60); // document word
    }
}
} // namespace

// Must be exactly this class: the friend declaration in codeeditor.h names
// AppleCat::Gui::LineNumberArea (an anonymous namespace would not match).
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), m_editor(editor)
    {
    }

    QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *m_editor;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setCursorWidth(2);

    m_lineNumberArea = new LineNumberArea(this);

    m_highlighter = new AppleCat::Core::RuleSyntaxHighlighter(
        document(), nullptr);

    m_autoPopupTimer = new QTimer(this);
    m_autoPopupTimer->setSingleShot(true);
    m_autoPopupTimer->setInterval(350);
    connect(m_autoPopupTimer, &QTimer::timeout, this, [this] { requestCompletion(true); });

    connect(this, &QPlainTextEdit::blockCountChanged, this,
            &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::onCursorMoved);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditor::onTextChanged);

    m_popup = new CompletionPopup(this);
    connect(m_popup, &CompletionPopup::rowActivated, this,
            [this](int index) { insertCompletionForIndex(index); });

    applySettings();
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::setSyntaxDefinition(
    std::shared_ptr<const AppleCat::Core::SyntaxDefinition> def)
{
    m_highlighter->setDefinition(def);
}

void CodeEditor::applySettings()
{
    const auto *s = AppleCat::Core::AppSettings::instance();

    QFont f(s->value(AppleCat::Core::AppSettings::kEditorFontFamily).toString(),
            s->value(AppleCat::Core::AppSettings::kEditorFontSize).toInt());
    setFont(f);
    m_lineNumberArea->setFont(f);

    setLineWrapMode(s->value(AppleCat::Core::AppSettings::kWordWrap).toBool()
                        ? QPlainTextEdit::WidgetWidth
                        : QPlainTextEdit::NoWrap);
    m_lineNumbersVisible =
        s->value(AppleCat::Core::AppSettings::kShowLineNumbers).toBool();

    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(s->value(AppleCat::Core::AppSettings::kColorEditorBg).toString()));
    setPalette(p);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::columnNumber() const
{
    return textCursor().columnNumber() + 1;
}

void CodeEditor::setLoading(bool loading)
{
    m_loading = loading;
}

void CodeEditor::setFindSelections(const QList<QTextEdit::ExtraSelection> &selections)
{
    m_findSelections = selections;
    highlightCurrentLine();
}

// ---------------------------------------------------------------------
// Completion

QString CodeEditor::wordBeforeCursor() const
{
    const QTextCursor tc = textCursor();
    const int pos = tc.positionInBlock();
    const QString line = tc.block().text();
    int start = pos;
    while (start > 0) {
        const QChar c = line.at(start - 1);
        if (c.isLetterOrNumber() || c == QLatin1Char('_') || c == QLatin1Char('.'))
            --start;
        else
            break;
    }
    return line.mid(start, pos - start);
}

void CodeEditor::rebuildDocWords()
{
    static constexpr int kMaxScanned = 4 * 1024 * 1024; // chars
    static constexpr int kMaxWords = 20000;
    const QString text = toPlainText().left(kMaxScanned);
    m_docWords = AppleCat::Core::CompletionEngine::extractWords(text, kMaxWords);
    m_docWordsDirty = false;
}

void CodeEditor::requestCompletion(bool autoTriggered)
{
    if (m_loading || isReadOnly())
        return;

    const auto opt = AppleCat::Core::CompletionEngine::instance()->options();
    if (!opt.enabled)
        return;
    if (autoTriggered && !opt.autoPopup)
        return;

    const QString prefix = wordBeforeCursor();
    if (prefix.size() < opt.minPrefix && !(prefix.size() >= 1 && !autoTriggered)) {
        hidePopup();
        return;
    }
    // A dot is not a good completion prefix on its own.
    if (prefix.endsWith(QLatin1Char('.')) || prefix.size() == 1 && !prefix.at(0).isLetterOrNumber()) {
        hidePopup();
        return;
    }

    if (m_docWordsDirty && opt.docWords)
        rebuildDocWords();

    m_currentCandidates = AppleCat::Core::CompletionEngine::instance()->compute(
        prefix, m_filePath, opt.docWords ? &m_docWords : nullptr);
    if (m_currentCandidates.isEmpty()) {
        hidePopup();
        return;
    }

    QList<QPair<QString, QPair<QString, QColor>>> rows;
    rows.reserve(m_currentCandidates.size());
    for (const auto &c : m_currentCandidates)
        rows.append({c.text, {c.detail, kindColor(c.kind)}});
    m_popup->setRows(rows);
    m_popup->select(0);

    QRect cr = cursorRect();
    const QPoint global = viewport()->mapToGlobal(cr.bottomLeft());
    m_popup->presentAt(global + QPoint(0, 2), 260);
}

void CodeEditor::hidePopup()
{
    m_popup->hide();
    m_currentCandidates.clear();
}

bool CodeEditor::popupVisible() const
{
    return m_popup->isVisible();
}

void CodeEditor::insertCompletionForIndex(int index)
{
    if (index < 0 || index >= m_currentCandidates.size())
        return;
    const auto cand = m_currentCandidates.at(index);

    // Replace the typed prefix.
    QTextCursor tc = textCursor();
    const QString prefix = wordBeforeCursor();
    if (!prefix.isEmpty()) {
        for (int i = 0; i < prefix.size(); ++i)
            tc.deletePreviousChar();
    }

    if (cand.kind == 1) { // snippet: expand body, honour ${cursor} and ${name} placeholders
        const auto snips =
            AppleCat::Core::CompletionEngine::instance()->snippetsFor(m_filePath);
        QString body = cand.text;
        for (const auto &s : snips) {
            if (s.prefix == cand.text) {
                body = s.body;
                break;
            }
        }

        int cursorPos = -1;
        const int cursorMarker = body.indexOf(QStringLiteral("${cursor}"));
        if (cursorMarker >= 0) {
            body.remove(cursorMarker, QStringLiteral("${cursor}").size());
            cursorPos = cursorMarker;
        }

        tc.insertText(body);

        // Land on the first remaining ${placeholder} with it selected so the
        // next keystroke replaces it; otherwise fall back to ${cursor}.
        static const QRegularExpression placeholder(QStringLiteral("\\$\\{([^}]*)\\}"));
        const auto pm = placeholder.match(body);
        if (pm.hasMatch()) {
            QTextCursor sel = tc;
            sel.setPosition(tc.position() - (body.size() - pm.capturedStart()));
            sel.setPosition(sel.position() + pm.capturedLength(), QTextCursor::KeepAnchor);
            setTextCursor(sel);
        } else if (cursorPos >= 0) {
            tc.setPosition(tc.position() - (body.size() - cursorPos));
            setTextCursor(tc);
        }
    } else {
        tc.insertText(cand.text);
    }
    setTextCursor(tc);
    hidePopup();
}

// ---------------------------------------------------------------------
// Key handling

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    // While the completion popup is up it owns navigation keys.
    if (popupVisible()) {
        switch (e->key()) {
        case Qt::Key_Down:
            m_popup->moveNext();
            return;
        case Qt::Key_Up:
            m_popup->movePrev();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            insertCompletionForIndex(m_popup->selected());
            return;
        case Qt::Key_Escape:
            hidePopup();
            emit completionAborted();
            return;
        default:
            break; // regular editing continues below
        }
    }

    if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Space) {
        requestCompletion(false);
        return;
    }

    const auto *s = AppleCat::Core::AppSettings::instance();

    if (e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
        indentSelection(false);
        return;
    }
    if (e->key() == Qt::Key_Backtab) {
        indentSelection(true);
        return;
    }

    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
        && e->modifiers() == Qt::NoModifier
        && s->value(AppleCat::Core::AppSettings::kAutoIndent).toBool()) {
        smartNewLine();
        return;
    }

    // Bracket / quote auto-close and skip-over.
    if (s->value(AppleCat::Core::AppSettings::kAutoCloseBrackets).toBool()) {
        const QString text = e->text();
        if (text.size() == 1) {
            static const QHash<QChar, QChar> pairs = {
                {QLatin1Char('('), QLatin1Char(')')},
                {QLatin1Char('['), QLatin1Char(']')},
                {QLatin1Char('{'), QLatin1Char('}')},
                {QLatin1Char('"'), QLatin1Char('"')},
                {QLatin1Char('\''), QLatin1Char('\'')},
            };
            const QChar ch = text.at(0);
            const bool isOpener = pairs.contains(ch);
            const bool isCloser = pairs.values().contains(ch);

            if (isOpener) {
                QTextCursor tc = textCursor();
                if (tc.hasSelection()) { // wrap the selection
                    const QString sel = tc.selectedText();
                    tc.insertText(QString(ch) + sel + pairs.value(ch));
                    tc.setPosition(tc.position() - 1 - sel.size());
                    tc.setPosition(tc.position() + sel.size(), QTextCursor::KeepAnchor);
                    setTextCursor(tc);
                    return;
                }
                // Skip auto-close when typing a quote next to a word char.
                const bool quote = ch == QLatin1Char('"') || ch == QLatin1Char('\'');
                QTextCursor probe = tc;
                probe.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                const bool afterWord = quote && !probe.selectedText().isEmpty()
                                       && probe.selectedText().at(0).isLetterOrNumber();
                if (!afterWord) {
                    tc.insertText(QString(ch) + pairs.value(ch));
                    tc.movePosition(QTextCursor::PreviousCharacter);
                    setTextCursor(tc);
                    return;
                }
            } else if (isCloser) {
                // Skip over an existing closer directly at the cursor.
                QTextCursor probe = textCursor();
                probe.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                if (probe.selectedText() == QString(ch)) {
                    probe.clearSelection();
                    setTextCursor(probe);
                    return;
                }
            }
        }
    }

    QPlainTextEdit::keyPressEvent(e);
}

void CodeEditor::focusInEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusInEvent(e);
}

void CodeEditor::focusOutEvent(QFocusEvent *e)
{
    hidePopup();
    QPlainTextEdit::focusOutEvent(e);
}

bool CodeEditor::event(QEvent *e)
{
    // Hide the popup when clicking somewhere else in the editor.
    if (e->type() == QEvent::MouseButtonPress && popupVisible())
        hidePopup();
    return QPlainTextEdit::event(e);
}

void CodeEditor::smartNewLine()
{
    QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    const int posInBlock = cursor.positionInBlock();
    const QString left = line.left(posInBlock);
    const QString right = line.mid(posInBlock);

    QString indent;
    for (const QChar &c : left) {
        if (c == QLatin1Char(' ') || c == QLatin1Char('\t'))
            indent += c;
        else
            break;
    }

    const QString trimmed = left.trimmed();
    const bool afterOpener = trimmed.endsWith(QLatin1Char('{'))
                             || trimmed.endsWith(QLatin1Char('('))
                             || trimmed.endsWith(QLatin1Char('['))
                             || trimmed.endsWith(QLatin1Char(':'));

    const auto *s = AppleCat::Core::AppSettings::instance();
    const int tabWidth = s->value(AppleCat::Core::AppSettings::kTabWidth).toInt();
    const QString oneIndent(tabWidth, QLatin1Char(' '));

    if (afterOpener) {
        const QChar last = trimmed.right(1).at(0);
        QChar closer;
        if (last == QLatin1Char('{'))
            closer = QLatin1Char('}');
        else if (last == QLatin1Char('('))
            closer = QLatin1Char(')');
        else if (last == QLatin1Char('['))
            closer = QLatin1Char(']');
        const QString trimmedRight = right.trimmed();
        if (!closer.isNull() && trimmedRight.startsWith(closer)) {
            // Sandwich: newline+indent+tab, newline+indent, cursor in the middle.
            cursor.insertText(QLatin1Char('\n') + indent + oneIndent + QLatin1Char('\n')
                              + indent);
            cursor.movePosition(QTextCursor::Up);
            cursor.movePosition(QTextCursor::EndOfLine);
            setTextCursor(cursor);
            return;
        }
        indent += oneIndent;
    }

    cursor.insertText(QLatin1Char('\n') + indent);
    setTextCursor(cursor);
}

void CodeEditor::indentSelection(bool dedent)
{
    const auto *s = AppleCat::Core::AppSettings::instance();
    const int tabWidth = s->value(AppleCat::Core::AppSettings::kTabWidth).toInt();
    const QString oneIndent(tabWidth, QLatin1Char(' '));

    QTextCursor tc = textCursor();
    if (!tc.hasSelection()
        || !tc.selectedText().contains(QChar(QChar::ParagraphSeparator))) {
        // Single caret: insert or remove leading spaces.
        if (!dedent) {
            tc.insertText(oneIndent);
            return;
        }
        QTextCursor line = tc;
        line.movePosition(QTextCursor::StartOfBlock);
        line.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          qMin(tabWidth, line.block().text().size()));
        if (line.selectedText().trimmed().isEmpty())
            line.removeSelectedText();
        return;
    }

    int selStart = tc.selectionStart();
    int selEnd = tc.selectionEnd();
    QTextCursor docCur(document());
    docCur.beginEditBlock();

    QTextBlock block = document()->findBlock(selStart);
    QTextBlock lastBlock = document()->findBlock(selEnd);
    while (block.isValid() && block.position() <= lastBlock.position()) {
        QTextCursor c(block);
        if (!dedent) {
            c.insertText(oneIndent);
            selEnd += oneIndent.size();
        } else {
            const QString t = block.text();
            int remove = 0;
            while (remove < tabWidth && remove < t.size()
                   && t.at(remove) == QLatin1Char(' '))
                ++remove;
            if (remove > 0) {
                c.setPosition(block.position() + remove);
                c.setPosition(block.position(), QTextCursor::KeepAnchor);
                c.removeSelectedText();
                selStart = qMax(selStart - remove, block.position());
                selEnd -= remove;
            }
        }
        block = block.next();
    }
    docCur.endEditBlock();

    QTextCursor restore(document());
    restore.setPosition(selStart);
    restore.setPosition(selEnd, QTextCursor::KeepAnchor);
    setTextCursor(restore);
}

QString CodeEditor::tabSpaces() const
{
    const auto *s = AppleCat::Core::AppSettings::instance();
    return QString(s->value(AppleCat::Core::AppSettings::kTabWidth).toInt(),
                   QLatin1Char(' '));
}

// ---------------------------------------------------------------------
// Text-change / cursor hooks

void CodeEditor::onTextChanged()
{
    if (m_loading)
        return;
    m_docWordsDirty = true;
    if (m_autoPopupTimer)
        m_autoPopupTimer->start();
}

void CodeEditor::onCursorMoved()
{
    highlightCurrentLine();
    if (popupVisible()) {
        // Follow the prefix while the popup is open; hide when the word ended.
        const auto opt = AppleCat::Core::CompletionEngine::instance()->options();
        const QString prefix = wordBeforeCursor();
        if (prefix.size() < qMax(1, opt.minPrefix) || prefix.endsWith(QLatin1Char('.')))
            hidePopup();
        else
            requestCompletion(true);
    }
}

// ---------------------------------------------------------------------
// Line numbers & current line

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 6 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

int CodeEditor::extraAreaWidth()
{
    return m_lineNumbersVisible ? lineNumberAreaWidth() : 0;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(extraAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), m_lineNumbersVisible ? lineNumberAreaWidth() : 0,
              cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;

    const auto *s = AppleCat::Core::AppSettings::instance();
    if (!isReadOnly() && s
        && s->value(AppleCat::Core::AppSettings::kHighlightCurrentLine).toBool()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(
            QColor(s->value(AppleCat::Core::AppSettings::kColorCurrentLine).toString()));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        selections.append(sel);
    }

    selections += m_findSelections;
    setExtraSelections(selections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    const auto *s = AppleCat::Core::AppSettings::instance();
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(),
                     QColor(s->value(AppleCat::Core::AppSettings::kColorLineNumBg).toString()));
    painter.setPen(QColor(s->value(AppleCat::Core::AppSettings::kColorLineNumFg).toString()));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, m_lineNumberArea->width() - 3,
                             fontMetrics().height(), Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

} // namespace AppleCat::Gui
