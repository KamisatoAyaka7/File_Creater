#include "findreplacebar.h"

#include "appsettings.h"
#include "codeeditor.h"

#include <QCheckBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

namespace AppleCat::Gui {

namespace {

// Esc closes the bar instead of clearing the line edit.
class EscFilter : public QObject
{
public:
    explicit EscFilter(FindReplaceBar *bar)
        : QObject(bar), m_bar(bar)
    {
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(event);
            if (ke->key() == Qt::Key_Escape) {
                emit m_bar->closeRequested();
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    FindReplaceBar *m_bar;
};

} // namespace

FindReplaceBar::FindReplaceBar(CodeEditor *editor)
    : QWidget(editor->parentWidget()), m_editor(editor)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 2, 6, 2);
    root->setSpacing(2);

    // Find row ------------------------------------------------------------
    auto *findRow = new QHBoxLayout;
    findRow->setSpacing(4);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText(tr("Find"));
    m_findEdit->setClearButtonEnabled(true);
    findRow->addWidget(m_findEdit, 1);

    auto *prevBtn = new QPushButton(tr("Previous"), this);
    prevBtn->setToolTip(tr("Previous match (Shift+Enter)"));
    auto *nextBtn = new QPushButton(tr("Next"), this);
    nextBtn->setToolTip(tr("Next match (Enter)"));
    auto *closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setToolTip(tr("Close (Esc)"));
    findRow->addWidget(prevBtn);
    findRow->addWidget(nextBtn);
    findRow->addWidget(closeBtn);

    // Replace row (hidden until Ctrl+H) ------------------------------------
    m_replaceRow = new QWidget(this);
    auto *replaceRow = new QHBoxLayout(m_replaceRow);
    replaceRow->setContentsMargins(0, 0, 0, 0);
    replaceRow->setSpacing(4);

    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("Replace with"));
    replaceRow->addWidget(m_replaceEdit, 1);

    auto *replaceBtn = new QPushButton(tr("Replace"), this);
    auto *replaceAllBtn = new QPushButton(tr("Replace all"), this);
    replaceRow->addWidget(replaceBtn);
    replaceRow->addWidget(replaceAllBtn);

    // Options row -----------------------------------------------------------
    auto *optRow = new QHBoxLayout;
    optRow->setSpacing(8);
    m_caseCheck = new QCheckBox(tr("Match case"), this);
    m_wordCheck = new QCheckBox(tr("Whole words"), this);
    m_regexCheck = new QCheckBox(tr("Regex"), this);
    m_countLabel = new QLabel(this);
    optRow->addWidget(m_caseCheck);
    optRow->addWidget(m_wordCheck);
    optRow->addWidget(m_regexCheck);
    optRow->addStretch(1);
    optRow->addWidget(m_countLabel);

    root->addLayout(findRow);
    root->addWidget(m_replaceRow);
    root->addLayout(optRow);

    // Wiring ------------------------------------------------------------------
    connect(nextBtn, &QPushButton::clicked, this, &FindReplaceBar::findNext);
    connect(prevBtn, &QPushButton::clicked, this, &FindReplaceBar::findPrev);
    connect(closeBtn, &QPushButton::clicked, this, &FindReplaceBar::closeRequested);
    connect(replaceBtn, &QPushButton::clicked, this, &FindReplaceBar::replaceOne);
    connect(replaceAllBtn, &QPushButton::clicked, this, &FindReplaceBar::replaceAll);

    connect(m_findEdit, &QLineEdit::returnPressed, this, [this] {
        if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
            findPrev();
        else
            findNext();
    });
    connect(m_replaceEdit, &QLineEdit::returnPressed, this,
            &FindReplaceBar::replaceOne);

    auto *escFilter = new EscFilter(this);
    m_findEdit->installEventFilter(escFilter);
    m_replaceEdit->installEventFilter(escFilter);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(250);

    connect(m_findEdit, &QLineEdit::textChanged, this, &FindReplaceBar::scheduleHighlight);
    connect(m_caseCheck, &QCheckBox::toggled, this, &FindReplaceBar::scheduleHighlight);
    connect(m_wordCheck, &QCheckBox::toggled, this, &FindReplaceBar::scheduleHighlight);
    connect(m_regexCheck, &QCheckBox::toggled, this, &FindReplaceBar::scheduleHighlight);
    connect(m_debounce, &QTimer::timeout, this, &FindReplaceBar::applyHighlightNow);

    // Keep highlights in sync while the document is edited (debounced).
    connect(m_editor, &CodeEditor::textChanged, this, &FindReplaceBar::scheduleHighlight);
    // Cursor moves only re-mark the current match; no rescan.
    connect(m_editor, &CodeEditor::cursorPositionChanged, this,
            &FindReplaceBar::updateSelections);

    if (auto *s = AppleCat::Core::AppSettings::instance()) {
        connect(s, &AppleCat::Core::AppSettings::settingsChanged, this,
                [this](const QStringList &keys) {
                    if (keys.contains(QLatin1String(AppleCat::Core::AppSettings::kColorFindHighlight))
                        || keys.contains(QLatin1String(AppleCat::Core::AppSettings::kColorFindCurrent))
                        || keys.contains(QLatin1String(AppleCat::Core::AppSettings::kFindHighlightAll))
                        || keys.contains(QLatin1String(AppleCat::Core::AppSettings::kFindMaxHighlights)))
                        applyHighlightNow();
                });
    }

    m_replaceRow->hide();
    hide();
}

void FindReplaceBar::activate(bool withReplace)
{
    show();
    m_replaceRow->setVisible(withReplace);
    m_findEdit->setFocus();
    m_findEdit->selectAll();
    applyHighlightNow();
}

FindReplaceBar::FindFlags FindReplaceBar::flags() const
{
    FindFlags f;
    f.caseSensitive = m_caseCheck->isChecked();
    f.wholeWord = m_wordCheck->isChecked();
    f.regex = m_regexCheck->isChecked();
    return f;
}

void FindReplaceBar::scheduleHighlight()
{
    if (!isVisible())
        return;
    m_debounce->start();
}

// Collects all matches (capped). Full rescan; only runs debounced.
void FindReplaceBar::applyHighlightNow()
{
    m_matches.clear();
    m_capped = false;

    if (!isVisible()) {
        updateSelections();
        return;
    }

    const auto *s = AppleCat::Core::AppSettings::instance();
    const int maxHighlights =
        s->value(AppleCat::Core::AppSettings::kFindMaxHighlights).toInt();

    const QString needle = m_findEdit->text();
    if (!needle.isEmpty()) {
        const FindFlags f = flags();
        if (f.regex) {
            QRegularExpression re(needle,
                                  f.caseSensitive
                                      ? QRegularExpression::NoPatternOption
                                      : QRegularExpression::CaseInsensitiveOption);
            if (re.isValid()) {
                const QString text = m_editor->toPlainText();
                auto it = re.globalMatch(text);
                while (it.hasNext()) {
                    const auto m = it.next();
                    const int len = m.capturedLength();
                    if (len == 0)
                        continue;
                    m_matches.append({int(m.capturedStart()), len});
                    if (m_matches.size() >= maxHighlights) {
                        m_capped = true;
                        break;
                    }
                }
            }
        } else {
            const QString text = m_editor->toPlainText();
            const Qt::CaseSensitivity cs = f.caseSensitive ? Qt::CaseSensitive
                                                           : Qt::CaseInsensitive;
            int pos = 0;
            while ((pos = text.indexOf(needle, pos, cs)) >= 0) {
                bool ok = true;
                if (f.wholeWord) {
                    const auto isWord = [](QChar c) {
                        return c.isLetterOrNumber() || c == QLatin1Char('_');
                    };
                    if (pos > 0 && isWord(text.at(pos - 1)))
                        ok = false;
                    if (ok && pos + needle.size() < text.size()
                        && isWord(text.at(pos + needle.size())))
                        ok = false;
                }
                if (ok) {
                    m_matches.append({pos, needle.size()});
                    if (m_matches.size() >= maxHighlights) {
                        m_capped = true;
                        break;
                    }
                }
                pos += 1;
            }
        }
    }

    updateSelections();
}

// Reuses m_matches; figures out the current one from the cursor and
// pushes the extra selections + count label.
void FindReplaceBar::updateSelections()
{
    const auto *s = AppleCat::Core::AppSettings::instance();

    m_currentMatch = -1;
    const int cursorPos = m_editor->textCursor().position();
    for (int i = 0; i < m_matches.size(); ++i) {
        if (cursorPos >= m_matches[i].first
            && cursorPos <= m_matches[i].first + m_matches[i].second) {
            m_currentMatch = i;
            break;
        }
    }

    QList<QTextEdit::ExtraSelection> selections;
    const bool highlightAll =
        s->value(AppleCat::Core::AppSettings::kFindHighlightAll).toBool();
    if (highlightAll && isVisible()) {
        const QColor all(
            s->value(AppleCat::Core::AppSettings::kColorFindHighlight).toString());
        const QColor cur(
            s->value(AppleCat::Core::AppSettings::kColorFindCurrent).toString());
        for (int i = 0; i < m_matches.size(); ++i) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(i == m_currentMatch ? cur : all);
            sel.cursor = QTextCursor(m_editor->document());
            sel.cursor.setPosition(m_matches[i].first);
            sel.cursor.setPosition(m_matches[i].first + m_matches[i].second,
                                   QTextCursor::KeepAnchor);
            selections.append(sel);
        }
    }
    m_editor->setFindSelections(selections);

    const QString needle = m_findEdit->text();
    if (!isVisible() || needle.isEmpty()) {
        m_countLabel->clear();
    } else if (m_matches.isEmpty()) {
        m_countLabel->setText(tr("no matches"));
    } else if (m_capped) {
        m_countLabel->setText(tr("%1+ matches").arg(m_matches.size()));
    } else if (m_currentMatch >= 0) {
        m_countLabel->setText(tr("%1/%2").arg(m_currentMatch + 1).arg(m_matches.size()));
    } else {
        m_countLabel->setText(tr("%1 matches").arg(m_matches.size()));
    }
}

bool FindReplaceBar::find(bool backward, QTextCursor *cursor)
{
    const QString needle = m_findEdit->text();
    if (needle.isEmpty())
        return false;

    QTextDocument::FindFlags docFlags;
    if (backward)
        docFlags |= QTextDocument::FindBackward;
    const FindFlags f = flags();
    if (f.caseSensitive)
        docFlags |= QTextDocument::FindCaseSensitively;
    if (f.wholeWord)
        docFlags |= QTextDocument::FindWholeWords;

    QRegularExpression re;
    if (f.regex) {
        re.setPattern(needle);
        if (f.caseSensitive)
            re.setPatternOptions(QRegularExpression::NoPatternOption);
        else
            re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) {
            emit statusMessage(tr("Invalid regular expression"));
            return false;
        }
    }

    auto doFind = [&](const QTextCursor &from) {
        return f.regex ? m_editor->document()->find(re, from, docFlags)
                       : m_editor->document()->find(needle, from, docFlags);
    };

    QTextCursor start = cursor ? *cursor : m_editor->textCursor();
    QTextCursor result = doFind(start);

    if (result.isNull()) {
        // Wrap around once so search is cyclic.
        QTextCursor wrapped(m_editor->document());
        if (backward)
            wrapped.movePosition(QTextCursor::End);
        result = doFind(wrapped);
    }

    if (!result.isNull()) {
        m_editor->setTextCursor(result);
        m_editor->centerCursor();
        applyHighlightNow();
        return true;
    }
    return false;
}

void FindReplaceBar::findNext()
{
    if (!find(false))
        emit statusMessage(tr("No matches for \"%1\"").arg(m_findEdit->text()));
}

void FindReplaceBar::findPrev()
{
    if (!find(true))
        emit statusMessage(tr("No matches for \"%1\"").arg(m_findEdit->text()));
}

void FindReplaceBar::replaceOne()
{
    const QString needle = m_findEdit->text();
    if (needle.isEmpty())
        return;

    QTextCursor cursor = m_editor->textCursor();
    // If the cursor already sits on a match, replace that; else find first.
    if (!cursor.hasSelection() || cursor.selectedText() != needle) {
        if (!find(false))
            return;
        cursor = m_editor->textCursor();
    }

    const QString replacement = m_replaceEdit->text();
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.insertText(replacement);
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);

    find(false); // jump to the next match
    emit statusMessage(tr("Replaced one occurrence"));
}

void FindReplaceBar::replaceAll()
{
    const QString needle = m_findEdit->text();
    if (needle.isEmpty())
        return;

    const FindFlags f = flags();
    const QString replacement = m_replaceEdit->text();
    const QString text = m_editor->toPlainText();

    // Collect all matches first, then apply edits from the end backwards so
    // earlier positions stay valid — one edit block keeps it one undo step.
    struct Match
    {
        int pos;
        int len;
    };
    QList<Match> matches;

    QRegularExpression re;
    if (f.regex) {
        re.setPattern(needle);
        if (f.caseSensitive)
            re.setPatternOptions(QRegularExpression::NoPatternOption);
        else
            re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) {
            emit statusMessage(tr("Invalid regular expression"));
            return;
        }
        auto it = re.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            if (m.capturedLength() == 0)
                continue;
            matches.append({int(m.capturedStart()), int(m.capturedLength())});
        }
    } else {
        const Qt::CaseSensitivity cs = f.caseSensitive ? Qt::CaseSensitive
                                                       : Qt::CaseInsensitive;
        int pos = 0;
        while ((pos = text.indexOf(needle, pos, cs)) >= 0) {
            matches.append({pos, needle.size()});
            pos += needle.size();
        }
    }

    if (matches.isEmpty()) {
        emit statusMessage(tr("No matches for \"%1\"").arg(needle));
        return;
    }

    QTextCursor docCursor(m_editor->document());
    docCursor.beginEditBlock();
    int count = 0;
    for (int i = matches.size() - 1; i >= 0; --i) {
        const Match &m = matches.at(i);
        QString insertText = replacement;
        if (f.regex) {
            const auto rm = re.match(text, m.pos, QRegularExpression::NormalMatch,
                                     QRegularExpression::AnchoredMatchOption);
            for (int g = 1; g <= 9; ++g)
                insertText.replace(QStringLiteral("\\") + QString::number(g),
                                   rm.captured(g));
        }
        QTextCursor c(m_editor->document());
        c.setPosition(m.pos);
        c.setPosition(m.pos + m.len, QTextCursor::KeepAnchor);
        c.removeSelectedText();
        c.insertText(insertText);
        ++count;
    }
    docCursor.endEditBlock();

    emit statusMessage(tr("Replaced %1 occurrence(s)").arg(count));
    scheduleHighlight();
}

} // namespace AppleCat::Gui
