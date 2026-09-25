// Asynchronous file IO. Every load/save operation gets its own dedicated
// thread, so opening a huge file never blocks the GUI thread. Results and
// progress are delivered back through queued signal/slot connections
// (automatically dropped when the receiving tab is destroyed).

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <functional>

namespace AppleCat::Core {

struct TextLoadResult
{
    bool ok = false;
    QString error;       // fatal error; empty when ok
    QString warning;     // non-fatal (e.g. replaced bytes)
    QString text;
    QString encoding;    // effective encoding id
    bool hadBom = false;
    qint64 fileSize = 0;
};

// `context` is a QObject living in the caller's (usually GUI) thread; the
// callbacks are invoked there. `encodingId` may be EncodingService::AUTO.
void loadTextFileAsync(const QString &path,
                       const QString &encodingId,
                       QObject *context,
                       std::function<void(TextLoadResult)> done,
                       std::function<void(qint64 bytes, qint64 total)> progress = {});

void saveTextFileAsync(const QString &path,
                       const QString &text,
                       const QString &encodingId,
                       bool writeBom,
                       QObject *context,
                       std::function<void(bool ok, QString error)> done);

void loadBytesAsync(const QString &path,
                    QObject *context,
                    std::function<void(bool ok, QByteArray data, QString error)> done,
                    std::function<void(qint64 bytes, qint64 total)> progress = {});

void saveBytesAsync(const QString &path,
                    const QByteArray &data,
                    QObject *context,
                    std::function<void(bool ok, QString error)> done);

} // namespace AppleCat::Core
