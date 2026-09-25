// Non-modal find & replace bar embedded at the top of each editor tab.
// The editor below stays fully usable while the bar is open. All matches
// are highlighted live (debounced, capped); Enter/Shift+Enter walk
// through them; Esc closes the bar.

#pragma once

#include <QList>
#include <QPair>
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextCursor;
class QTimer;

namespace AppleCat::Gui {

class CodeEditor;

class FindReplaceBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindReplaceBar(CodeEditor *editor);

    // Shows (if needed), focuses the find field and selects its text.
    void activate(bool withReplace);

signals:
    void closeRequested();
    void statusMessage(const QString &msg);

public slots:
    void findNext();
    void findPrev();
    void replaceOne();
    void replaceAll();

private slots:
    void scheduleHighlight();
    void applyHighlightNow(); // full rescan (debounced)
    void updateSelections();  // cheap: re-mark current match only

private:
    struct FindFlags
    {
        bool caseSensitive = false;
        bool wholeWord = false;
        bool regex = false;
    };
    FindFlags flags() const;

    bool find(bool backward, QTextCursor *cursor = nullptr);
    void refreshColors();

    CodeEditor *m_editor;
    QLineEdit *m_findEdit = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QWidget *m_replaceRow = nullptr;
    QCheckBox *m_caseCheck = nullptr;
    QCheckBox *m_wordCheck = nullptr;
    QCheckBox *m_regexCheck = nullptr;
    QLabel *m_countLabel = nullptr;
    QTimer *m_debounce = nullptr;

    QList<QPair<int, int>> m_matches; // capped positions
    int m_currentMatch = -1;
    bool m_capped = false;
};

} // namespace AppleCat::Gui
