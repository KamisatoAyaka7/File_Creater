#include "encodingservice.h"

#include <QHash>
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringEncoder>
#include <utility>

#ifdef Q_OS_WIN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

namespace AppleCat::Core {

const QString EncodingService::AUTO = QStringLiteral("auto");

static EncodingService *g_instance = nullptr;

EncodingService &EncodingService::instance()
{
    if (!g_instance) {
        g_instance = new EncodingService();
        g_instance->init();
    }
    return *g_instance;
}

// ---------------------------------------------------------------------
// Windows code page helpers

#ifdef Q_OS_WIN

static bool cpToUnicode(UINT cp, const QByteArray &raw, QString *out)
{
    if (raw.isEmpty()) {
        out->clear();
        return true;
    }
    const int inLen = int(raw.size());
    const int needed = MultiByteToWideChar(cp, 0, raw.constData(), inLen, nullptr, 0);
    if (needed <= 0)
        return false;
    std::wstring w(size_t(needed), L'\0');
    const int written = MultiByteToWideChar(cp, 0, raw.constData(), inLen, w.data(), needed);
    if (written <= 0)
        return false;
    w.resize(size_t(written));
    *out = QString::fromStdWString(w);
    return true;
}

static bool unicodeToCp(UINT cp, const QString &text, QByteArray *out)
{
    if (text.isEmpty()) {
        out->clear();
        return true;
    }
    const std::wstring w = text.toStdWString();
    const int wLen = int(w.size());
    const int needed = WideCharToMultiByte(cp, 0, w.data(), wLen, nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
        return false;
    QByteArray buf(needed, Qt::Uninitialized);
    const int written = WideCharToMultiByte(cp, 0, w.data(), wLen, buf.data(), needed,
                                            nullptr, nullptr);
    if (written <= 0)
        return false;
    buf.truncate(written);
    *out = buf;
    return true;
}

static bool cpValid(UINT cp)
{
    // Round-trip a probe string to see whether the OS knows this code page.
    QString uni;
    if (!cpToUnicode(cp, QByteArray("A"), &uni) || uni != QStringLiteral("A"))
        return false;
    QByteArray back;
    if (!unicodeToCp(cp, uni, &back))
        return false;
    return true;
}

static bool cpValidateStrict(UINT cp, const QByteArray &raw)
{
    if (raw.isEmpty())
        return true;
    const int r = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS,
                                      raw.constData(), int(raw.size()), nullptr, 0);
    if (r > 0)
        return true;
    const DWORD err = GetLastError();
    if (err == ERROR_NO_UNICODE_TRANSLATION)
        return false;
    // Some code pages do not support MB_ERR_INVALID_CHARS; treat them as valid.
    return true;
}

static UINT activeCodePage()
{
#ifdef Q_OS_WIN
    return GetACP();
#else
    return 0;
#endif
}

#endif // Q_OS_WIN

// ---------------------------------------------------------------------

void EncodingService::init()
{
    m_infos.clear();
    m_entries.clear();
    m_systemName = QStringLiteral("System");

    addUnicode(QStringLiteral("UTF-8"),      QStringLiteral("UTF-8"));
    addUnicode(QStringLiteral("UTF-16LE"),   QStringLiteral("UTF-16 LE"));
    addUnicode(QStringLiteral("UTF-16BE"),   QStringLiteral("UTF-16 BE"));
    addUnicode(QStringLiteral("UTF-32LE"),   QStringLiteral("UTF-32 LE"));
    addUnicode(QStringLiteral("UTF-32BE"),   QStringLiteral("UTF-32 BE"));
    addUnicode(QStringLiteral("ISO-8859-1"), QStringLiteral("ISO-8859-1 (Latin-1)"));

    addCodePage(QStringLiteral("GBK"),         QStringLiteral("GBK / GB2312"),         QStringLiteral("Chinese"),          936);
    addCodePage(QStringLiteral("GB18030"),     QStringLiteral("GB18030"),              QStringLiteral("Chinese"),          54936);
    addCodePage(QStringLiteral("Big5"),        QStringLiteral("Big5"),                 QStringLiteral("Chinese"),          950);
    addCodePage(QStringLiteral("Shift-JIS"),   QStringLiteral("Shift-JIS"),            QStringLiteral("Japanese"),         932);
    addCodePage(QStringLiteral("EUC-JP"),      QStringLiteral("EUC-JP"),               QStringLiteral("Japanese"),         20932);
    addCodePage(QStringLiteral("EUC-KR"),      QStringLiteral("EUC-KR / CP949"),       QStringLiteral("Korean"),           949);
    addCodePage(QStringLiteral("KOI8-R"),      QStringLiteral("KOI8-R"),               QStringLiteral("Cyrillic"),         20866);
    addCodePage(QStringLiteral("KOI8-U"),      QStringLiteral("KOI8-U"),               QStringLiteral("Cyrillic"),         21866);
    addCodePage(QStringLiteral("windows-1251"),QStringLiteral("Windows-1251"),        QStringLiteral("Cyrillic"),         1251);
    addCodePage(QStringLiteral("ISO-8859-5"),  QStringLiteral("ISO-8859-5"),          QStringLiteral("Cyrillic"),         28595);
    addCodePage(QStringLiteral("windows-1252"),QStringLiteral("Windows-1252"),        QStringLiteral("Western European"), 1252);
    addCodePage(QStringLiteral("ISO-8859-15"), QStringLiteral("ISO-8859-15"),         QStringLiteral("Western European"), 28605);
    addCodePage(QStringLiteral("windows-1250"),QStringLiteral("Windows-1250"),        QStringLiteral("Central European"), 1250);
    addCodePage(QStringLiteral("ISO-8859-2"),  QStringLiteral("ISO-8859-2"),          QStringLiteral("Central European"), 28592);
    addCodePage(QStringLiteral("windows-1257"),QStringLiteral("Windows-1257"),        QStringLiteral("Baltic"),           1257);
    addCodePage(QStringLiteral("ISO-8859-13"), QStringLiteral("ISO-8859-13"),         QStringLiteral("Baltic"),           28603);
    addCodePage(QStringLiteral("windows-1253"),QStringLiteral("Windows-1253"),        QStringLiteral("Greek"),            1253);
    addCodePage(QStringLiteral("ISO-8859-7"),  QStringLiteral("ISO-8859-7"),          QStringLiteral("Greek"),            28597);
    addCodePage(QStringLiteral("windows-1254"),QStringLiteral("Windows-1254"),        QStringLiteral("Turkish"),          1254);
    addCodePage(QStringLiteral("ISO-8859-9"),  QStringLiteral("ISO-8859-9"),          QStringLiteral("Turkish"),          28599);
    addCodePage(QStringLiteral("windows-1255"),QStringLiteral("Windows-1255"),        QStringLiteral("Hebrew"),           1255);
    addCodePage(QStringLiteral("ISO-8859-8"),  QStringLiteral("ISO-8859-8"),          QStringLiteral("Hebrew"),           28598);
    addCodePage(QStringLiteral("windows-1256"),QStringLiteral("Windows-1256"),        QStringLiteral("Arabic"),           1256);
    addCodePage(QStringLiteral("ISO-8859-6"),  QStringLiteral("ISO-8859-6"),          QStringLiteral("Arabic"),           28596);
    addCodePage(QStringLiteral("windows-874"),  QStringLiteral("Windows-874"),         QStringLiteral("Thai"),             874);
    addCodePage(QStringLiteral("windows-1258"),QStringLiteral("Windows-1258"),        QStringLiteral("Vietnamese"),       1258);

    addSystem();
}

void EncodingService::addUnicode(const QString &id, const QString &name)
{
    Entry e;
    e.info.id = id;
    e.info.name = name;
    e.info.group = QStringLiteral("Unicode");
    e.info.unicode = true;
    m_infos.append(e.info);
    m_entries.insert(id, e);
}

void EncodingService::addCodePage(const QString &id, const QString &name,
                                   const QString &group, int cp)
{
#ifdef Q_OS_WIN
    if (!cpValid(UINT(cp)))
        return;
#endif
    Entry e;
    e.info.id = id;
    e.info.name = name;
    e.info.group = group;
    e.info.unicode = false;
    e.codePage = cp;
    m_infos.append(e.info);
    m_entries.insert(id, e);
}

void EncodingService::addSystem()
{
    Entry e;
    e.info.id = QStringLiteral("System");
    e.info.group = QStringLiteral("Unicode");
    e.info.unicode = false;
#ifdef Q_OS_WIN
    e.codePage = int(activeCodePage());
    // Give the entry a friendly name such as "System (GBK)" when the active
    // code page matches one of the registered encodings.
    for (const Info &i : std::as_const(m_infos)) {
        const auto it = m_entries.constFind(i.id);
        if (it != m_entries.constEnd() && it->codePage == e.codePage) {
            m_systemName = QStringLiteral("System (%1)").arg(i.name);
            break;
        }
    }
    if (m_systemName == QStringLiteral("System"))
        m_systemName = QStringLiteral("System (CP %1)").arg(e.codePage);
#else
    e.codePage = 0;
#endif
    e.info.name = m_systemName;
    m_infos.prepend(e.info);
    m_entries.insert(e.info.id, e);
}

EncodingService::Info EncodingService::info(const QString &id) const
{
    return m_entries.value(id).info;
}

QString EncodingService::displayName(const QString &id) const
{
    const Info i = info(id);
    return i.isNull() ? id : i.name;
}

// Returns the BOM length for `id` when `raw` starts with one, else 0.
static int matchingBom(const QString &id, const QByteArray &raw)
{
    auto startsWith = [&raw](const char *bom, int len) {
        return raw.size() >= len && QByteArray(raw.constData(), len) == QByteArray(bom, len);
    };
    if (id == QLatin1String("UTF-8")) {
        if (startsWith("\xEF\xBB\xBF", 3))
            return 3;
    } else if (id == QLatin1String("UTF-16LE")) {
        if (startsWith("\xFF\xFE", 2))
            return 2;
    } else if (id == QLatin1String("UTF-16BE")) {
        if (startsWith("\xFE\xFF", 2))
            return 2;
    } else if (id == QLatin1String("UTF-32LE")) {
        if (startsWith("\xFF\xFE\x00\x00", 4))
            return 4;
    } else if (id == QLatin1String("UTF-32BE")) {
        if (startsWith("\x00\x00\xFE\xFF", 4))
            return 4;
    }
    return 0;
}

static QStringConverter::Encoding converterFor(const QString &id, bool *ok)
{
    *ok = true;
    if (id == QLatin1String("UTF-8"))      return QStringConverter::Utf8;
    if (id == QLatin1String("UTF-16LE"))   return QStringConverter::Utf16LE;
    if (id == QLatin1String("UTF-16BE"))   return QStringConverter::Utf16BE;
    if (id == QLatin1String("UTF-32LE"))   return QStringConverter::Utf32LE;
    if (id == QLatin1String("UTF-32BE"))   return QStringConverter::Utf32BE;
    if (id == QLatin1String("ISO-8859-1")) return QStringConverter::Latin1;
    if (id == QLatin1String("System"))     return QStringConverter::System;
    *ok = false;
    return QStringConverter::Utf8;
}

QString EncodingService::decode(const QByteArray &raw, const QString &id,
                                bool *hadBom, QString *error) const
{
    if (hadBom)
        *hadBom = false;
    if (error)
        error->clear();

    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd()) {
        if (error)
            *error = QStringLiteral("Unknown encoding: %1").arg(id);
        return QString();
    }

    // Unicode family via QStringConverter.
    bool isConverter = false;
    const QStringConverter::Encoding enc = converterFor(id, &isConverter);
    if (isConverter) {
        const int bom = matchingBom(id, raw);
        if (hadBom && bom)
            *hadBom = true;
        QStringDecoder decoder(enc);
        QString text = decoder.decode(QByteArrayView(raw).mid(bom));
        if (error && decoder.hasError())
            *error = QStringLiteral("Some byte sequences were not valid %1 "
                                    "and were replaced.").arg(displayName(id));
        return text;
    }

    // Legacy code page.
#ifdef Q_OS_WIN
    QString text;
    if (!cpToUnicode(UINT(it->codePage), raw, &text)) {
        if (error)
            *error = QStringLiteral("Failed to decode as %1.").arg(displayName(id));
        return QString::fromLatin1(raw);
    }
    return text;
#else
    Q_UNUSED(it);
    if (error)
        *error = QStringLiteral("Legacy encodings require Windows.");
    return QString::fromLatin1(raw);
#endif
}

QByteArray EncodingService::encode(const QString &text, const QString &id,
                                   bool writeBom, QString *error) const
{
    if (error)
        error->clear();

    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd()) {
        if (error)
            *error = QStringLiteral("Unknown encoding: %1").arg(id);
        return QByteArray();
    }

    bool isConverter = false;
    const QStringConverter::Encoding enc = converterFor(id, &isConverter);
    if (isConverter) {
        QStringEncoder encoder(enc);
        QByteArray bytes = encoder.encode(text);
        if (encoder.hasError()) {
            if (error)
                *error = QStringLiteral("Some characters cannot be represented in %1.")
                             .arg(displayName(id));
        }
        // Normalise the BOM: QStringEncoder may emit one on its own; strip
        // it, then prepend only if requested.
        const int bom = matchingBom(id, bytes);
        if (bom)
            bytes.remove(0, bom);
        if (writeBom && id != QLatin1String("ISO-8859-1") && id != QLatin1String("System")) {
            if (id == QLatin1String("UTF-8"))
                bytes.prepend(QByteArray("\xEF\xBB\xBF", 3));
            else if (id == QLatin1String("UTF-16LE"))
                bytes.prepend(QByteArray("\xFF\xFE", 2));
            else if (id == QLatin1String("UTF-16BE"))
                bytes.prepend(QByteArray("\xFE\xFF", 2));
            else if (id == QLatin1String("UTF-32LE"))
                bytes.prepend(QByteArray("\xFF\xFE\x00\x00", 4));
            else if (id == QLatin1String("UTF-32BE"))
                bytes.prepend(QByteArray("\x00\x00\xFE\xFF", 4));
        }
        return bytes;
    }

#ifdef Q_OS_WIN
    QByteArray bytes;
    if (!unicodeToCp(UINT(it->codePage), text, &bytes)) {
        if (error)
            *error = QStringLiteral("Failed to encode as %1.").arg(displayName(id));
        return text.toLatin1();
    }
    return bytes;
#else
    Q_UNUSED(it);
    if (error)
        *error = QStringLiteral("Legacy encodings require Windows.");
    return text.toLatin1();
#endif
}

bool EncodingService::validateLegacy(const QString &id, const QByteArray &raw) const
{
    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd() || it->codePage == 0)
        return false;
#ifdef Q_OS_WIN
    return cpValidateStrict(UINT(it->codePage), raw);
#else
    Q_UNUSED(raw);
    return false;
#endif
}

// Strict UTF-8 validation independent of QStringDecoder's error reporting
// (its stateless mode is not guaranteed to flag invalid sequences). Rejects
// overlong forms, surrogates and code points beyond U+10FFFF.
static bool strictUtf8(const QByteArray &raw)
{
    const auto *d = reinterpret_cast<const uchar *>(raw.constData());
    const qsizetype n = raw.size();
    qsizetype i = 0;
    while (i < n) {
        const uchar c = d[i];
        if (c < 0x80) {
            ++i;
            continue;
        }
        int extra = 0;
        if ((c & 0xE0) == 0xC0)
            extra = 1;
        else if ((c & 0xF0) == 0xE0)
            extra = 2;
        else if ((c & 0xF8) == 0xF0)
            extra = 3;
        else
            return false;
        if (i + extra >= n)
            return false;
        for (int k = 1; k <= extra; ++k) {
            if ((d[i + k] & 0xC0) != 0x80)
                return false;
        }
        // Reject overlong encodings and surrogate code points.
        if (extra == 1 && c < 0xC2)
            return false;
        if (extra == 2) {
            if (c == 0xE0 && d[i + 1] < 0xA0)
                return false;
            if (c == 0xED && d[i + 1] >= 0xA0)
                return false;
        }
        if (extra == 3) {
            if (c == 0xF0 && d[i + 1] < 0x90)
                return false;
            if (c > 0xF4 || (c == 0xF4 && d[i + 1] >= 0x90))
                return false;
        }
        i += extra + 1;
    }
    return true;
}

EncodingService::Detection EncodingService::detect(const QByteArray &raw,
                                                   const QString &legacyFallback) const
{
    Detection d;
    d.id = QStringLiteral("UTF-8");

    auto startsWith = [&raw](const char *bom, int len) {
        return raw.size() >= len && QByteArray(raw.constData(), len) == QByteArray(bom, len);
    };

    if (startsWith("\xFF\xFE\x00\x00", 4)) {
        d.id = QStringLiteral("UTF-32LE");
        d.hadBom = true;
        return d;
    }
    if (startsWith("\x00\x00\xFE\xFF", 4)) {
        d.id = QStringLiteral("UTF-32BE");
        d.hadBom = true;
        return d;
    }
    if (startsWith("\xEF\xBB\xBF", 3)) {
        d.id = QStringLiteral("UTF-8");
        d.hadBom = true;
        return d;
    }
    if (startsWith("\xFF\xFE", 2)) {
        d.id = QStringLiteral("UTF-16LE");
        d.hadBom = true;
        return d;
    }
    if (startsWith("\xFE\xFF", 2)) {
        d.id = QStringLiteral("UTF-16BE");
        d.hadBom = true;
        return d;
    }

    // BOM-less UTF-16 heuristic: ASCII-heavy UTF-16 has a zero byte at
    // every other position.
    if (raw.size() >= 64 && raw.size() % 2 == 0) {
        qsizetype evenZeros = 0, oddZeros = 0;
        const qsizetype pairs = raw.size() / 2;
        for (qsizetype i = 0; i < pairs; ++i) {
            if (raw[2 * i] == '\0')
                ++evenZeros;
            if (raw[2 * i + 1] == '\0')
                ++oddZeros;
        }
        const double evenRatio = double(evenZeros) / double(pairs);
        const double oddRatio = double(oddZeros) / double(pairs);
        if (oddRatio > 0.6 && evenRatio < 0.1) {
            d.id = QStringLiteral("UTF-16LE");
            return d;
        }
        if (evenRatio > 0.6 && oddRatio < 0.1) {
            d.id = QStringLiteral("UTF-16BE");
            return d;
        }
    }

    // Strict UTF-8 validation.
    if (strictUtf8(raw)) {
        d.id = QStringLiteral("UTF-8");
        return d;
    }

    // Configured legacy fallback — but only when it is an explicit choice.
    // A single-byte "System" code page (e.g. Windows-1252) matches almost
    // any byte sequence, so it must not shadow the multi-byte candidates
    // below; it is retried at the end instead.
    if (isValid(legacyFallback) && legacyFallback != QStringLiteral("System")
        && validateLegacy(legacyFallback, raw)) {
        d.id = legacyFallback;
        return d;
    }

    // Common multi-byte encodings in order of preference.
    const QStringList candidates = {
        QStringLiteral("GBK"), QStringLiteral("Big5"), QStringLiteral("Shift-JIS"),
        QStringLiteral("EUC-KR"), QStringLiteral("windows-1252"),
    };
    for (const QString &c : candidates) {
        if (c == legacyFallback)
            continue;
        if (isValid(c) && validateLegacy(c, raw)) {
            d.id = c;
            return d;
        }
    }

    // Single-byte fallback ("System" or an explicit single-byte choice).
    if (isValid(legacyFallback) && validateLegacy(legacyFallback, raw)) {
        d.id = legacyFallback;
        return d;
    }

    return d; // UTF-8 with replacements
}

} // namespace AppleCat::Core
