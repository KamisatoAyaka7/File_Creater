#include "asyncio.h"

#include "appsettings.h"
#include "encodingservice.h"

#include <QFile>
#include <QFileDevice>
#include <QSaveFile>
#include <QThread>

namespace AppleCat::Core {

namespace {

// One QObject living inside a per-operation thread. The work lambda runs
// on `started`; results are emitted as queued signals back to the GUI
// thread, which makes them safe against receiver destruction.
// (Qt signals are protected, so public emit wrappers are provided for the
// worker lambda which is not a member of the class.)
class IoJob : public QObject
{
    Q_OBJECT

public:
    std::function<void()> work; // performs IO and emits one of the signals

    void emitTextLoaded(bool ok, const QString &error, const QString &warning,
                        const QString &text, const QString &encoding, bool hadBom,
                        qint64 fileSize)
    {
        emit textLoaded(ok, error, warning, text, encoding, hadBom, fileSize);
    }
    void emitTextSaved(bool ok, const QString &error) { emit textSaved(ok, error); }
    void emitBytesLoaded(bool ok, const QByteArray &data, const QString &error)
    {
        emit bytesLoaded(ok, data, error);
    }
    void emitBytesSaved(bool ok, const QString &error) { emit bytesSaved(ok, error); }
    void emitProgress(qint64 done, qint64 total) { emit progress(done, total); }

public slots:
    void run()
    {
        if (work)
            work();
        thread()->quit();
    }

signals:
    void textLoaded(bool ok, const QString &error, const QString &warning,
                    const QString &text, const QString &encoding, bool hadBom,
                    qint64 fileSize);
    void textSaved(bool ok, const QString &error);
    void bytesLoaded(bool ok, const QByteArray &data, const QString &error);
    void bytesSaved(bool ok, const QString &error);
    void progress(qint64 done, qint64 total);
};

QThread *spawnJob(IoJob *job)
{
    QThread *thread = new QThread;
    job->moveToThread(thread);
    QObject::connect(thread, &QThread::started, job, &IoJob::run);
    QObject::connect(thread, &QThread::finished, job, &QObject::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return thread;
}

QByteArray readAllWithProgress(const QString &path, IoJob *job,
                               QString *errorOut, qint64 *sizeOut)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *errorOut = QStringLiteral("Cannot open file: %1").arg(file.errorString());
        return QByteArray();
    }
    *sizeOut = file.size();

    static constexpr qint64 Chunk = 4 * 1024 * 1024;
    QByteArray raw;
    raw.reserve(int(qMin<qint64>(file.size(), 64 * 1024 * 1024)));
    qint64 read = 0;
    while (true) {
        const QByteArray chunk = file.read(Chunk);
        if (chunk.isEmpty())
            break;
        raw.append(chunk);
        read += chunk.size();
        job->emitProgress(read, file.size());
    }
    if (file.error() != QFileDevice::NoError) {
        *errorOut = QStringLiteral("Read error: %1").arg(file.errorString());
        return QByteArray();
    }
    return raw;
}

} // namespace

void loadTextFileAsync(const QString &path, const QString &encodingId, QObject *context,
                       std::function<void(TextLoadResult)> done,
                       std::function<void(qint64, qint64)> progress)
{
    IoJob *job = new IoJob;

    QObject::connect(job, &IoJob::textLoaded, context,
                     [done](bool ok, const QString &error, const QString &warning,
                            const QString &text, const QString &encoding, bool hadBom,
                            qint64 fileSize) {
                         TextLoadResult r;
                         r.ok = ok;
                         r.error = error;
                         r.warning = warning;
                         r.text = text;
                         r.encoding = encoding;
                         r.hadBom = hadBom;
                         r.fileSize = fileSize;
                         done(r);
                     });
    if (progress)
        QObject::connect(job, &IoJob::progress, context,
                         [progress](qint64 doneBytes, qint64 total) {
                             progress(doneBytes, total);
                         });

    // Resolve settings in the caller's thread (QSettings is not thread-safe).
    const QString fallback = AppSettings::instance()
                                 ? AppSettings::instance()->value(AppSettings::kEncodingFallback).toString()
                                 : QStringLiteral("System");

    job->work = [job, path, encodingId, fallback] {
        TextLoadResult r;
        QString ioError;
        qint64 size = 0;
        const QByteArray raw = readAllWithProgress(path, job, &ioError, &size);
        r.fileSize = size;
        if (!ioError.isEmpty()) {
            job->emitTextLoaded(false, ioError, QString(), QString(), QString(), false, size);
            return;
        }

        QString effective = encodingId;
        if (effective == EncodingService::AUTO || effective.isEmpty())
            effective = EncodingService::instance().detect(raw, fallback).id;

        bool hadBom = false;
        QString warning;
        const QString text = EncodingService::instance().decode(raw, effective, &hadBom, &warning);
        job->emitTextLoaded(true, QString(), warning, text, effective, hadBom, size);
    };
    spawnJob(job);
}

void saveTextFileAsync(const QString &path, const QString &text, const QString &encodingId,
                       bool writeBom, QObject *context,
                       std::function<void(bool, QString)> done)
{
    IoJob *job = new IoJob;
    QObject::connect(job, &IoJob::textSaved, context,
                     [done](bool ok, const QString &error) { done(ok, error); });

    job->work = [job, path, text, encodingId, writeBom] {
        QString encError;
        const QByteArray bytes = EncodingService::instance().encode(text, encodingId,
                                                                    writeBom, &encError);
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            job->emitTextSaved(false, QStringLiteral("Cannot write file: %1")
                                                 .arg(file.errorString()));
            return;
        }
        if (file.write(bytes) != bytes.size()) {
            file.cancelWriting();
            job->emitTextSaved(false, QStringLiteral("Write error: %1")
                                                 .arg(file.errorString()));
            return;
        }
        if (!file.commit()) {
            job->emitTextSaved(false, QStringLiteral("Commit failed: %1")
                                                 .arg(file.errorString()));
            return;
        }
        job->emitTextSaved(true, encError);
    };
    spawnJob(job);
}

void loadBytesAsync(const QString &path, QObject *context,
                    std::function<void(bool, QByteArray, QString)> done,
                    std::function<void(qint64, qint64)> progress)
{
    IoJob *job = new IoJob;
    QObject::connect(job, &IoJob::bytesLoaded, context,
                     [done](bool ok, const QByteArray &data, const QString &error) {
                         done(ok, data, error);
                     });
    if (progress)
        QObject::connect(job, &IoJob::progress, context,
                         [progress](qint64 doneBytes, qint64 total) {
                             progress(doneBytes, total);
                         });

    job->work = [job, path] {
        QString error;
        qint64 size = 0;
        const QByteArray raw = readAllWithProgress(path, job, &error, &size);
        if (!error.isEmpty())
            job->emitBytesLoaded(false, QByteArray(), error);
        else
            job->emitBytesLoaded(true, raw, QString());
    };
    spawnJob(job);
}

void saveBytesAsync(const QString &path, const QByteArray &data, QObject *context,
                    std::function<void(bool, QString)> done)
{
    IoJob *job = new IoJob;
    QObject::connect(job, &IoJob::bytesSaved, context,
                     [done](bool ok, const QString &error) { done(ok, error); });

    job->work = [job, path, data] {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            job->emitBytesSaved(false, QStringLiteral("Cannot write file: %1")
                                                   .arg(file.errorString()));
            return;
        }
        if (file.write(data) != data.size()) {
            file.cancelWriting();
            job->emitBytesSaved(false, QStringLiteral("Write error: %1")
                                                   .arg(file.errorString()));
            return;
        }
        if (!file.commit()) {
            job->emitBytesSaved(false, QStringLiteral("Commit failed: %1")
                                                   .arg(file.errorString()));
            return;
        }
        job->emitBytesSaved(true, QString());
    };
    spawnJob(job);
}

} // namespace AppleCat::Core

#include "asyncio.moc"
