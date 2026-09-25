// Central code editor: line numbers, current-line highlight, auto indent,
// bracket auto-close, Tab-as-spaces and the completion popup.

#pragma once

#include <QPlainTextEdit>
#include <QSet>
#include <memory>

#include "completion.h"

class QListWidget;
class QListWidgetItem;
class QTimer;

namespace AppleCat::Core {
class RuleSyntaxHighlighter;
struct SyntaxDefinition;
}

namespace AppleCat::Gui {

class CompletionPopup;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void setSyntaxDefinition(std::shared_ptr<const AppleCat::Core::SyntaxDefinition> def);
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path) { m_filePath = path; }

    void applySettings(); // re-reads everything from AppSettings

    // Number of characters from the start of the line to the cursor.
    int columnNumber() const;

    // Search highlights are merged with the current-line highlight; the
    // find bar pushes its selections here.
    void setFindSelections(const QList<QTextEdit::ExtraSelection> &selections);

    void setLoading(bool loading);

    // Ctrl+Space entry point from the main window's action.
    void requestCompletionPublic() { requestCompletion(false); }

signals:
    void completionAborted(); // popup closed without inserting (Esc)

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    bool event(QEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onTextChanged();
    void onCursorMoved();

private:
    // Completion plumbing --------------------------------------------------
    void requestCompletion(bool autoTriggered);
    QString wordBeforeCursor() const;
    void insertCompletionForIndex(int index);
    void hidePopup();
    bool popupVisible() const;

    void smartNewLine();
    void indentSelection(bool dedent);
    QString tabSpaces() const;
    void rebuildDocWords();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    int extraAreaWidth();

    QWidget *m_lineNumberArea = nullptr;
    AppleCat::Core::RuleSyntaxHighlighter *m_highlighter = nullptr;
    CompletionPopup *m_popup = nullptr;
    QTimer *m_autoPopupTimer = nullptr;
    QList<AppleCat::Core::CompletionCandidate> m_currentCandidates;

    QString m_filePath;
    QSet<QString> m_docWords;
    bool m_docWordsDirty = true;
    bool m_loading = false; // suppress completion while chunk-filling
    bool m_lineNumbersVisible = true;
    QList<QTextEdit::ExtraSelection> m_findSelections;

    friend class LineNumberArea;
};

} // namespace AppleCat::Gui
