// One text editor tab: async load/save with progress, encoding + BOM
// management, progressive (chunked) fill for huge documents so the GUI
// stays responsive, and the embedded find/replace bar.

#pragma once

#include "docview.h"
#include "asyncio.h"
#include "findreplacebar.h"

namespace AppleCat::Gui {

class CodeEditor;
class FindReplaceBar;

class EditorTab : public DocView
{
    Q_OBJECT

public:
    explicit EditorTab(QWidget *parent = nullptr);

    // Starts an async load. encodingId may be EncodingService::AUTO.
    void load(const QString &path, const QString &encodingId);
    // Shows static text (help, about) without touching the filesystem.
    void loadTextDirect(const QString &title, const QString &text);
    void reload();

    void setEncodingAndReload(const QString &encodingId);
    void setBomWanted(bool bom) { m_bomWanted = bom; }
    bool bomWanted() const { return m_bomWanted; }
    QString encoding() const { return m_encoding; }

    void activateFind(bool withReplace);
    void hideFindBar() { m_findBar->hide(); }

    CodeEditor *editor() const { return m_editor; }

    // DocView
    QString filePath() const override { return m_filePath; }
    QString title() const override;
    QString toolTip() const override { return m_filePath; }
    bool isDirty() const override;
    bool isLoading() const override { return m_loading; }
    void save() override;
    void saveAs() override;
    void applySettings() override;

signals:
    void cursorMoved(int line, int col);
    void infoChanged(); // encoding / size / load state changed

private:
    void onLoadDone(const AppleCat::Core::TextLoadResult &result);
    void beginProgressiveFill(const QString &text);
    void fillChunk();
    void finishFill();
    void applySyntaxForPath();
    void doSave(const QString &path);

    CodeEditor *m_editor = nullptr;
    FindReplaceBar *m_findBar = nullptr;

    QString m_filePath;
    QString m_encoding;       // effective encoding id
    bool m_hadBom = false;
    bool m_bomWanted = false;
    bool m_loading = false;
    int m_loadPercent = 0;
    bool m_saving = false;

    // Progressive fill state
    QString m_fillText;
    int m_fillPos = 0;
    QString m_displayTitle;   // for loadTextDirect
};

} // namespace AppleCat::Gui
