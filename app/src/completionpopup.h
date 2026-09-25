#pragma once

#include <QColor>
#include <QPair>
#include <QString>
#include <QWidget>

class QListWidget;

namespace AppleCat::Gui {

// Lightweight completion popup: a frameless list anchored below the
// editor cursor. The editor feeds it candidate rows and handles keys; the
// popup only draws and reports clicks.
class CompletionPopup : public QWidget
{
    Q_OBJECT

public:
    explicit CompletionPopup(QWidget *editor);

    // rows: (display text, detail, color)
    void setRows(const QList<QPair<QString, QPair<QString, QColor>>> &rows);
    void select(int index);
    int selected() const;
    void moveNext();
    void movePrev();

    // Sizes the popup to its content and places it at `globalPos`,
    // flipping above the cursor when there is no room below.
    void presentAt(const QPoint &globalPos, int maxHeight);

    QString rowText(int index) const;     // text of the selected row
    bool rowIsSnippet(int index) const;

signals:
    void rowActivated(int index);

private:
    QListWidget *m_list;
    QList<bool> m_isSnippet;
};

} // namespace AppleCat::Gui
