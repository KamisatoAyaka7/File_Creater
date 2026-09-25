// Common interface for anything shown as a tab (text editor, hex editor).

#pragma once

#include <QWidget>

namespace AppleCat::Gui {

class DocView : public QWidget
{
    Q_OBJECT

public:
    explicit DocView(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }
    ~DocView() override = default;

    virtual QString filePath() const = 0;
    virtual QString title() const = 0;     // tab text (with dirty marker)
    virtual QString toolTip() const = 0;
    virtual bool isDirty() const = 0;
    virtual bool isLoading() const = 0;
    virtual void save() = 0;
    virtual void saveAs() = 0;
    virtual void applySettings() = 0;      // live re-apply of AppSettings

signals:
    void changed();                 // title / dirty state changed
    void statusMessage(const QString &msg);
    void saveFinished(bool ok);     // async save done
};

} // namespace AppleCat::Gui
