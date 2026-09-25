#include "completionpopup.h"

#include <QApplication>

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QScreen>

namespace AppleCat::Gui {

CompletionPopup::CompletionPopup(QWidget *editor)
    : QWidget(editor, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    // WindowDoesNotAcceptFocus keeps the keyboard on the editor: without it
    // the popup would steal focus on Windows and swallow every keystroke
    // typed while it is visible.

    m_list = new QListWidget(this);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setUniformItemSizes(true);
    m_list->setMouseTracking(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        emit rowActivated(m_list->row(item));
    });
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit rowActivated(m_list->row(item));
    });
}

void CompletionPopup::setRows(const QList<QPair<QString, QPair<QString, QColor>>> &rows)
{
    m_list->clear();
    m_isSnippet.clear();
    for (const auto &row : rows) {
        auto *item = new QListWidgetItem(row.first, m_list);
        // Keep the raw insert text retrievable even when a detail is shown.
        item->setData(Qt::AccessibleTextRole, row.first);
        if (!row.second.first.isEmpty()) {
            item->setText(QStringLiteral("%1  %2").arg(row.first, row.second.first));
            QFont f = item->font();
            f.setItalic(true);
            item->setData(Qt::FontRole, f);
        }
        item->setForeground(row.second.second);
        m_isSnippet.append(false);
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void CompletionPopup::select(int index)
{
    if (index >= 0 && index < m_list->count())
        m_list->setCurrentRow(index);
}

int CompletionPopup::selected() const
{
    return m_list->currentRow();
}

void CompletionPopup::moveNext()
{
    const int n = m_list->count();
    if (n == 0)
        return;
    m_list->setCurrentRow((m_list->currentRow() + 1) % n);
}

void CompletionPopup::movePrev()
{
    const int n = m_list->count();
    if (n == 0)
        return;
    m_list->setCurrentRow((m_list->currentRow() - 1 + n) % n);
}

void CompletionPopup::presentAt(const QPoint &globalPos, int maxHeight)
{
    const int w = qMax(260, m_list->sizeHintForColumn(0) + 40);
    int h = m_list->sizeHintForRow(0) * qMin(m_list->count(), 10) + 8;
    h = qMin(h, maxHeight);

    QPoint pos = globalPos;
    const QScreen *screen = QApplication::screenAt(globalPos);
    const QRect avail = screen ? screen->availableGeometry()
                               : QApplication::primaryScreen()->availableGeometry();
    if (pos.y() + h > avail.bottom())
        pos.setY(pos.y() - h - 20); // flip above the cursor line
    if (pos.x() + w > avail.right())
        pos.setX(avail.right() - w);

    setGeometry(QRect(pos, QSize(w, h)));
    show();
}

QString CompletionPopup::rowText(int index) const
{
    if (index < 0 || index >= m_list->count())
        return QString();
    // The display text is "candidate  detail"; the insert text is the
    // candidate part stored as the item's accessible name.
    QListWidgetItem *item = m_list->item(index);
    return item->data(Qt::AccessibleTextRole).toString();
}

bool CompletionPopup::rowIsSnippet(int index) const
{
    return index >= 0 && index < m_isSnippet.size() && m_isSnippet.at(index);
}

} // namespace AppleCat::Gui
