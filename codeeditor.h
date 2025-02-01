#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QCompleter>
#include <QMap>
#include <QColor>

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void setCompleter(QCompleter *completer);
    QCompleter *completer() const;

    static void setGlobalKeywords(const QMap<QString, QColor> &keywords);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

private slots:
    void insertCompletion(const QString &completion);

private:
    QString textUnderCursor() const;
    void handleIndentation();

    QCompleter *m_completer;
    static QMap<QString, QColor> globalKeywords;
};

#endif // CODEEDITOR_H
