// Encoding registry and conversion service.
//
// Unicode encodings are handled by QStringConverter. Legacy 8-bit and
// double-byte encodings (GBK, Big5, Shift-JIS, EUC-KR, KOI8-R, the
// windows-125x family, ...) are delegated to the operating system's code
// page support (MultiByteToWideChar / WideCharToMultiByte on Windows),
// which keeps the binary free of huge code tables while still exposing a
// large catalogue of encodings.

#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

namespace AppleCat::Core {

class EncodingService
{
public:
    struct Info
    {
        QString id;      // stable internal id, e.g. "UTF-8", "GBK", "windows-1251"
        QString name;    // display name
        QString group;   // grouping for menus
        bool unicode = false;
        bool isNull() const { return id.isEmpty(); }
    };

    // Special id accepted by file loading: pick automatically.
    static const QString AUTO;

    static EncodingService &instance();

    // Enumerates and validates all supported encodings. Call once at
    // startup (from the GUI thread); afterwards the service is read-only
    // and safe to use from worker threads.
    void init();

    QList<Info> encodings() const { return m_infos; }
    bool isValid(const QString &id) const { return m_entries.contains(id); }
    Info info(const QString &id) const;
    QString displayName(const QString &id) const;

    // Decodes raw bytes. A leading BOM matching the encoding is stripped
    // and reported through `hadBom`. Invalid sequences are replaced;
    // `error` (if given) receives a warning message in that case.
    QString decode(const QByteArray &raw, const QString &id,
                   bool *hadBom = nullptr, QString *error = nullptr) const;

    // Encodes text. When `writeBom` is true a matching BOM is prepended
    // (UTF-8/16/32 only). `error` is set when the target encoding cannot
    // represent the text.
    QByteArray encode(const QString &text, const QString &id, bool writeBom,
                      QString *error = nullptr) const;

    struct Detection
    {
        QString id;
        bool hadBom = false;
    };

    // BOM / heuristic detection used when the requested encoding is "auto":
    //   1. explicit BOM
    //   2. UTF-16 null-density heuristic (for BOM-less UTF-16)
    //   3. strict UTF-8 validation
    //   4. the configured legacy fallback
    //   5. strict validation of common legacy encodings (GBK, Big5,
    //      Shift-JIS, EUC-KR, windows-1252)
    //   6. UTF-8 with replacement characters
    Detection detect(const QByteArray &raw, const QString &legacyFallback) const;

    // Strict validity check for a legacy code page (used by detection).
    bool validateLegacy(const QString &id, const QByteArray &raw) const;

private:
    struct Entry
    {
        Info info;
        int codePage = 0; // 0 => QStringConverter-based (or System)
    };

    void addUnicode(const QString &id, const QString &name);
    void addCodePage(const QString &id, const QString &name, const QString &group, int cp);
    void addSystem();

    QList<Info> m_infos;
    QHash<QString, Entry> m_entries;
    QString m_systemName; // resolved display name, e.g. "System (GBK)"

    friend class EncodingServiceTests;
};

} // namespace AppleCat::Core
