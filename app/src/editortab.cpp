#include "editortab.h"

#include "appsettings.h"
#include "asyncio.h"
#include "codeeditor.h"
#include "encodingservice.h"
#include "findreplacebar.h"
#include "syntaxrepository.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QVBoxLayout>

namespace AppleCat::Gui {

static constexpr int kChunkChars = 512 * 1024;     // per fill tick
static constexpr int kProgressiveThreshold = 1024 * 1024; // fill progressively above 1 MB
static constexpr qint64 kMaxFileBytes = 256 * 1024 * 1024; // hard cap

EditorTab::EditorTab(QWidget *parent)
    : DocView(parent)
{
    m_editor = new CodeEditor(this);
    m_findBar = new FindReplaceBar(m_editor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_findBar);
    layout->addWidget(m_editor);

    connect(m_editor, &CodeEditor::modificationChanged, this, &DocView::changed);
    connect(m_editor, &CodeEditor::cursorPositionChanged, this, [this] {
        emit cursorMoved(m_editor->textCursor().blockNumber() + 1,
                         m_editor->columnNumber());
    });
    connect(m_findBar, &FindReplaceBar::closeRequested, m_findBar, &QWidget::hide);
    connect(m_findBar, &FindReplaceBar::statusMessage, this, &DocView::statusMessage);

    m_encoding = AppleCat::Core::AppSettings::instance()
                     ->value(AppleCat::Core::AppSettings::kEncodingDefaultSave)
                     .toString();
}

void EditorTab::load(const QString &path, const QString &encodingId)
{
    QFileInfo info(path);
    if (info.size() > kMaxFileBytes) {
        emit statusMessage(tr("Refusing to open %1: larger than %2 MB")
                               .arg(QFileInfo(path).fileName())
                               .arg(kMaxFileBytes / (1024 * 1024)));
        return;
    }

    m_filePath = path;
    m_displayTitle.clear();
    m_loading = true;
    m_loadPercent = 0;
    m_editor->setLoading(true);
    // The completion engine keys keyword/snippet lookups on this path.
    m_editor->setFilePath(path);
    emit changed();

    AppleCat::Core::loadTextFileAsync(
        path, encodingId, this,
        [this](const AppleCat::Core::TextLoadResult r) { onLoadDone(r); },
        [this](qint64 done, qint64 total) {
            m_loadPercent = total > 0 ? int(done * 100 / total) : 100;
            emit changed();
        });
}

void EditorTab::loadTextDirect(const QString &title, const QString &text)
{
    m_filePath = QString();
    m_displayTitle = title;
    m_editor->setPlainText(text);
    m_editor->document()->setModified(false);
    emit changed();
}

void EditorTab::onLoadDone(const AppleCat::Core::TextLoadResult &result)
{
    m_loading = false;
    m_loadPercent = 100;

    if (!result.ok) {
        m_editor->setLoading(false);
        emit statusMessage(tr("Load failed: %1").arg(result.error));
        emit changed();
        return;
    }

    m_encoding = result.encoding;
    m_hadBom = result.hadBom;
    m_bomWanted = result.hadBom;

    applySyntaxForPath();

    if (!result.warning.isEmpty())
        emit statusMessage(result.warning);

    if (result.text.size() > kProgressiveThreshold) {
        beginProgressiveFill(result.text);
    } else {
        m_editor->setPlainText(result.text);
        m_editor->document()->setModified(false);
        m_editor->setLoading(false);
    }

    AppleCat::Core::AppSettings::instance()->pushRecentFile(m_filePath);
    emit infoChanged();
    emit changed();
}

void EditorTab::beginProgressiveFill(const QString &text)
{
    m_fillText = text;
    m_fillPos = 0;
    m_editor->clear();
    m_editor->document()->setUndoRedoEnabled(false);
    fillChunk();
}

void EditorTab::fillChunk()
{
    const int len = qMin(kChunkChars, m_fillText.size() - m_fillPos);
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(m_fillText.mid(m_fillPos, len));
    m_fillPos += len;

    m_loadPercent = 50 + int(m_fillPos * 50 / qMax(1, m_fillText.size()));
    emit changed();

    if (m_fillPos < m_fillText.size()) {
        QTimer::singleShot(0, this, &EditorTab::fillChunk);
        return;
    }
    finishFill();
}

void EditorTab::finishFill()
{
    if (!m_fillText.isNull()) {
        m_editor->document()->setUndoRedoEnabled(true);
        m_editor->document()->setModified(false);
        m_editor->setLoading(false);
        m_fillText = QString();
        m_loadPercent = 100;
    }
}

void EditorTab::reload()
{
    if (m_filePath.isEmpty())
        return;
    load(m_filePath, m_encoding);
}

void EditorTab::setEncodingAndReload(const QString &encodingId)
{
    m_encoding = encodingId;
    if (!m_filePath.isEmpty())
        reload();
    else
        emit infoChanged();
}

void EditorTab::applySyntaxForPath()
{
    m_editor->setSyntaxDefinition(
        AppleCat::Core::SyntaxRepository::instance().definitionForFile(m_filePath));
}

QString EditorTab::title() const
{
    QString base = !m_displayTitle.isEmpty()
                       ? m_displayTitle
                       : (m_filePath.isEmpty()
                              ? tr("Untitled")
                              : QFileInfo(m_filePath).fileName());
    if (m_loading)
        return QStringLiteral("[%1%] %2").arg(m_loadPercent).arg(base);
    if (m_editor->document()->isModified())
        base += QLatin1Char('*');
    return base;
}

bool EditorTab::isDirty() const
{
    return !m_loading && m_editor->document()->isModified();
}

void EditorTab::activateFind(bool withReplace)
{
    m_findBar->activate(withReplace);
}

void EditorTab::applySettings()
{
    m_editor->applySettings();
}

void EditorTab::save()
{
    if (m_filePath.isEmpty()) {
        saveAs();
        return;
    }
    doSave(m_filePath);
}

void EditorTab::saveAs()
{
    QString suggested = m_filePath;
    if (suggested.isEmpty())
        suggested = tr("Untitled.txt");
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), suggested, tr("All Files (*)"));
    if (path.isEmpty())
        return;

    m_filePath = path;
    m_displayTitle.clear();
    m_editor->setFilePath(path);
    applySyntaxForPath();
    AppleCat::Core::AppSettings::instance()->pushRecentFile(path);
    emit changed();
    doSave(path);
}

void EditorTab::doSave(const QString &path)
{
    if (m_saving)
        return;
    m_saving = true;
    emit statusMessage(tr("Saving %1...").arg(QFileInfo(path).fileName()));

    const QString text = m_editor->toPlainText();
    const QString enc = m_encoding;
    const bool bom = m_bomWanted;

    AppleCat::Core::saveTextFileAsync(
        path, text, enc, bom, this,
        [this, path](bool ok, const QString &error) {
            m_saving = false;
            if (ok) {
                m_editor->document()->setModified(false);
                QString msg = tr("Saved %1 (%2)").arg(QFileInfo(path).fileName(),
                    AppleCat::Core::EncodingService::instance().displayName(m_encoding));
                if (!error.isEmpty())
                    msg += QStringLiteral(" — ") + error;
                emit statusMessage(msg);
            } else {
                emit statusMessage(tr("Save failed: %1").arg(error));
            }
            emit changed();
            emit saveFinished(ok);
        });
}

} // namespace AppleCat::Gui
