// Hex editor opened as a tab in the main window. Fully editable: typing
// hex into a byte cell or a character into the ASCII column modifies the
// byte; changes are undoable and can be saved back to disk (async).

#pragma once

#include "docview.h"

#include <QAbstractTableModel>
#include <QByteArray>
#include <QStack>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableView;
class QToolButton;

namespace AppleCat::Gui {

class HexTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HexTableModel(QObject *parent = nullptr);

    void setData_(const QByteArray &data); // reset
    const QByteArray &bytes() const { return m_data; }
    void setBytesPerRow(int bpr);
    int bytesPerRow() const { return m_bpr; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // Direct programmatic byte write (undo records are kept by the owner).
    void setByteDirect(int offset, quint8 value);

    static constexpr int kAddressColumn = 0;

signals:
    void byteEditedByUser(int offset, quint8 oldValue, quint8 newValue);

private:
    bool byteColumnData(const QModelIndex &index, int role, QVariant *out) const;
    bool asciiColumnData(const QModelIndex &index, int role, QVariant *out) const;
    int asciiColumn() const { return 1 + m_bpr; }
    bool isAsciiColumn(int column) const { return column == asciiColumn(); }

    QByteArray m_data;
    int m_bpr = 16;
};

class HexEditor : public DocView
{
    Q_OBJECT

public:
    explicit HexEditor(const QString &path, QWidget *parent = nullptr);

    void load(const QString &path);

    // DocView
    QString filePath() const override { return m_filePath; }
    QString title() const override;
    QString toolTip() const override { return m_filePath; }
    bool isDirty() const override { return m_dirty; }
    bool isLoading() const override { return m_loading; }
    void save() override;
    void saveAs() override;
    void applySettings() override;

private:
    void gotoOffset(const QString &text);
    void undo();
    void rebuildUi();

    HexTableModel *m_model = nullptr;
    QTableView *m_view = nullptr;
    QComboBox *m_bprCombo = nullptr;
    QComboBox *m_radixCombo = nullptr;
    QToolButton *m_asciiToggle = nullptr;
    QToolButton *m_undoButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_gotoEdit = nullptr;

    QString m_filePath;
    bool m_loading = false;
    bool m_dirty = false;
    int m_loadPercent = 0;

    struct EditOp
    {
        int offset;
        quint8 oldValue;
    };
    QStack<EditOp> m_undoStack;
};

} // namespace AppleCat::Gui
