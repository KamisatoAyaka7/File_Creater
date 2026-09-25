#include "hexeditor.h"

#include "appsettings.h"
#include "asyncio.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSaveFile>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

namespace AppleCat::Gui {

// ---------------------------------------------------------------------
// Model

HexTableModel::HexTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void HexTableModel::setData_(const QByteArray &data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void HexTableModel::setBytesPerRow(int bpr)
{
    beginResetModel();
    m_bpr = bpr;
    endResetModel();
}

int HexTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return (m_data.size() + m_bpr - 1) / m_bpr;
}

int HexTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    // address + N byte columns + ascii column (single combined column)
    return 1 + m_bpr + 1;
}

Qt::ItemFlags HexTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    // Only the per-byte hex columns are editable; the combined ASCII column
    // is display-only so per-byte semantics stay unambiguous.
    if (index.column() >= 1 && index.column() <= m_bpr)
        f |= Qt::ItemIsEditable;
    return f;
}

QVariant HexTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid())
        return {};
    QVariant out;
    if (index.column() == kAddressColumn) {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const int offset = index.row() * m_bpr;
            return QStringLiteral("%1").arg(offset, 8, 16, QLatin1Char('0')).toUpper();
        }
        return {};
    }
    if (isAsciiColumn(index.column()))
        return asciiColumnData(index, role, &out) ? out : QVariant();
    return byteColumnData(index, role, &out) ? out : QVariant();
}

bool HexTableModel::byteColumnData(const QModelIndex &index, int role,
                                   QVariant *out) const
{
    const int offset = index.row() * m_bpr + (index.column() - 1);
    if (offset >= m_data.size())
        return false;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        *out = QStringLiteral("%1").arg(quint8(m_data[offset]), 2, 16,
                                        QLatin1Char('0')).toUpper();
        return true;
    }
    if (role == Qt::TextAlignmentRole) {
        *out = int(Qt::AlignCenter);
        return true;
    }
    return false;
}

bool HexTableModel::asciiColumnData(const QModelIndex &index, int role,
                                    QVariant *out) const
{
    if (role == Qt::DisplayRole) {
        QString s;
        for (int i = 0; i < m_bpr; ++i) {
            const int offset = index.row() * m_bpr + i;
            if (offset >= m_data.size())
                break;
            const quint8 b = quint8(m_data[offset]);
            s += (b >= 0x20 && b < 0x7f) ? QChar(b) : QChar(QLatin1Char('.'));
        }
        *out = s;
        return true;
    }
    if (role == Qt::TextAlignmentRole) {
        *out = int(Qt::AlignLeft | Qt::AlignVCenter);
        return true;
    }
    if (role == Qt::FontRole) {
        // Non-editable rendering uses the same mono font; nothing special.
        return false;
    }
    return false;
}

bool HexTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid())
        return false;

    if (isAsciiColumn(index.column()))
        return false; // display-only

    int offset = -1;
    quint8 newByte = 0;

    if (index.column() >= 1 && index.column() <= m_bpr) {
        offset = index.row() * m_bpr + (index.column() - 1);
        if (offset >= m_data.size())
            return false;
        bool okNum = false;
        const int v = value.toString().trimmed().toInt(&okNum, 16);
        if (!okNum || v < 0 || v > 0xff)
            return false;
        newByte = quint8(v);
    } else {
        return false;
    }

    const quint8 old = quint8(m_data[offset]);
    if (old == newByte)
        return false;
    m_data[offset] = char(newByte);
    emit dataChanged(index, index);
    emit byteEditedByUser(offset, old, newByte);
    return true;
}

void HexTableModel::setByteDirect(int offset, quint8 value)
{
    if (offset < 0 || offset >= m_data.size())
        return;
    m_data[offset] = char(value);
    const int row = offset / m_bpr;
    const int col = offset % m_bpr + 1;
    const QModelIndex idx = index(row, col);
    emit dataChanged(idx, idx);
}

// ---------------------------------------------------------------------
// View

HexEditor::HexEditor(const QString &path, QWidget *parent)
    : DocView(parent)
{
    m_model = new HexTableModel(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setShowGrid(false);
    m_view->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->verticalHeader()->hide();
    m_view->verticalHeader()->setDefaultSectionSize(20);
    m_view->horizontalHeader()->hide();
    m_view->setCornerButtonEnabled(false);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    toolbar->addWidget(new QLabel(tr("Bytes/row:"), this));
    m_bprCombo = new QComboBox(this);
    m_bprCombo->addItems({QStringLiteral("8"), QStringLiteral("16"),
                          QStringLiteral("24"), QStringLiteral("32")});
    m_bprCombo->setCurrentText(
        QString::number(AppleCat::Core::AppSettings::instance()
                            ->value(AppleCat::Core::AppSettings::kHexBytesPerRow)
                            .toInt()));
    toolbar->addWidget(m_bprCombo);

    toolbar->addWidget(new QLabel(tr("Address:"), this));
    m_radixCombo = new QComboBox(this);
    m_radixCombo->addItem(tr("Hex"));
    m_radixCombo->addItem(tr("Dec"));
    toolbar->addWidget(m_radixCombo);

    m_asciiToggle = new QToolButton(this);
    m_asciiToggle->setText(tr("ASCII"));
    m_asciiToggle->setCheckable(true);
    m_asciiToggle->setChecked(AppleCat::Core::AppSettings::instance()
                                  ->value(AppleCat::Core::AppSettings::kHexShowAscii)
                                  .toBool());
    toolbar->addWidget(m_asciiToggle);

    m_gotoEdit = new QLineEdit(this);
    m_gotoEdit->setPlaceholderText(tr("Go to offset (e.g. 0x100 or 256)"));
    m_gotoEdit->setMaximumWidth(220);
    toolbar->addWidget(m_gotoEdit);

    m_undoButton = new QToolButton(this);
    m_undoButton->setText(tr("Undo"));
    toolbar->addWidget(m_undoButton);

    auto *saveButton = new QToolButton(this);
    saveButton->setText(tr("Save"));
    toolbar->addWidget(saveButton);

    toolbar->addStretch(1);
    m_statusLabel = new QLabel(this);
    toolbar->addWidget(m_statusLabel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addLayout(toolbar);
    layout->addWidget(m_view, 1);

    connect(m_bprCombo, &QComboBox::currentTextChanged, this, [this](const QString &t) {
        m_model->setBytesPerRow(t.toInt());
        rebuildUi();
    });
    connect(m_asciiToggle, &QToolButton::toggled, this, &HexEditor::rebuildUi);
    connect(m_radixCombo, &QComboBox::currentIndexChanged, this, &HexEditor::rebuildUi);
    connect(m_gotoEdit, &QLineEdit::returnPressed, this,
            [this] { gotoOffset(m_gotoEdit->text()); });
    connect(m_undoButton, &QToolButton::clicked, this, &HexEditor::undo);
    connect(saveButton, &QToolButton::clicked, this, &DocView::save);

    connect(m_model, &HexTableModel::byteEditedByUser, this,
            [this](int offset, quint8 oldValue, quint8) {
                m_undoStack.push({offset, oldValue});
                m_undoButton->setEnabled(true);
                if (!m_dirty) {
                    m_dirty = true;
                    emit changed();
                }
            });

    applySettings();
    if (!path.isEmpty())
        load(path);
    rebuildUi();
}

void HexEditor::load(const QString &path)
{
    m_filePath = path;
    m_loading = true;
    m_loadPercent = 0;
    emit changed();

    AppleCat::Core::loadBytesAsync(
        path, this,
        [this](bool ok, const QByteArray &data, const QString &error) {
            m_loading = false;
            if (!ok) {
                emit statusMessage(tr("Hex load failed: %1").arg(error));
            } else {
                m_model->setData_(data);
                rebuildUi();
                emit statusMessage(tr("Loaded %1 bytes").arg(data.size()));
            }
            emit changed();
        },
        [this](qint64 done, qint64 total) {
            m_loadPercent = total > 0 ? int(done * 100 / total) : 100;
            emit changed();
        });
}

void HexEditor::rebuildUi()
{
    const int bpr = m_model->bytesPerRow();
    const QFont mono(
        AppleCat::Core::AppSettings::instance()
            ->value(AppleCat::Core::AppSettings::kEditorFontFamily)
            .toString(),
        AppleCat::Core::AppSettings::instance()
            ->value(AppleCat::Core::AppSettings::kEditorFontSize)
            .toInt());
    m_view->setFont(mono);

    const int digitW = QFontMetrics(mono).horizontalAdvance(QLatin1Char('9'));
    const int byteW = digitW * 2 + 12;
    const int addrW = digitW * 9;

    m_view->setColumnWidth(HexTableModel::kAddressColumn, addrW);
    for (int c = 1; c <= bpr; ++c)
        m_view->setColumnWidth(c, byteW);
    if (m_asciiToggle->isChecked()) {
        m_view->setColumnWidth(1 + bpr, digitW * bpr + 12);
    } else {
        m_view->setColumnWidth(1 + bpr, 0);
    }

    m_statusLabel->setText(
        tr("%1 bytes%2").arg(m_model->bytes().size())
            .arg(m_dirty ? tr(" · unsaved") : QString()));
    m_undoButton->setEnabled(!m_undoStack.isEmpty());
}

void HexEditor::gotoOffset(const QString &text)
{
    bool ok = false;
    const int offset = text.toInt(&ok, 0); // auto-detects 0x prefix
    if (!ok) {
        emit statusMessage(tr("Invalid offset"));
        return;
    }
    const int size = m_model->bytes().size();
    if (offset < 0 || offset >= size) {
        emit statusMessage(tr("Offset out of range (0..%1)").arg(size - 1));
        return;
    }
    const int row = offset / m_model->bytesPerRow();
    const int col = offset % m_model->bytesPerRow() + 1;
    const QModelIndex idx = m_model->index(row, col);
    m_view->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    m_view->setCurrentIndex(idx);
    emit statusMessage(tr("Went to offset %1 (0x%2)")
                           .arg(offset)
                           .arg(offset, 0, 16));
}

void HexEditor::undo()
{
    if (m_undoStack.isEmpty())
        return;
    const auto op = m_undoStack.pop();
    m_model->setByteDirect(op.offset, op.oldValue);
    rebuildUi();
    emit statusMessage(tr("Undid edit at offset %1").arg(op.offset));
}

QString HexEditor::title() const
{
    const QString base = m_filePath.isEmpty()
                             ? tr("Hex")
                             : QStringLiteral("%1 [hex]").arg(QFileInfo(m_filePath).fileName());
    if (m_loading)
        return QStringLiteral("[%1%] %2").arg(m_loadPercent).arg(base);
    return m_dirty ? base + QLatin1Char('*') : base;
}

void HexEditor::applySettings()
{
    const auto *s = AppleCat::Core::AppSettings::instance();
    const int bpr = s->value(AppleCat::Core::AppSettings::kHexBytesPerRow).toInt();
    if (m_bprCombo->currentText().toInt() != bpr)
        m_bprCombo->setCurrentText(QString::number(bpr));
    m_model->setBytesPerRow(m_bprCombo->currentText().toInt());
    rebuildUi();
}

void HexEditor::save()
{
    if (m_filePath.isEmpty()) {
        saveAs();
        return;
    }
    emit statusMessage(tr("Saving %1...").arg(QFileInfo(m_filePath).fileName()));
    const QByteArray snapshot = m_model->bytes();
    AppleCat::Core::saveBytesAsync(
        m_filePath, snapshot, this, [this, snapshot](bool ok, const QString &error) {
            if (ok) {
                m_dirty = false;
                emit statusMessage(tr("Saved %1 bytes").arg(snapshot.size()));
            } else {
                emit statusMessage(tr("Save failed: %1").arg(error));
            }
            emit changed();
            emit saveFinished(ok);
        });
}

void HexEditor::saveAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Binary As"), m_filePath, tr("All Files (*)"));
    if (path.isEmpty())
        return;
    m_filePath = path;
    emit changed();
    save();
}

} // namespace AppleCat::Gui
