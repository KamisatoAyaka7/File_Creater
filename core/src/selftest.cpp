#include "selftest.h"

#include "appsettings.h"
#include "asyncio.h"
#include "completion.h"
#include "encodingservice.h"
#include "syntaxhighlighter.h"
#include "syntaxrepository.h"

#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextDocument>
#include <QThread>
#include <QTimer>

namespace AppleCat::Core {

void SelfTest::check(bool cond, const QString &name, const QString &failMessage)
{
    if (cond) {
        m_allPassed = m_allPassed && true;
    } else {
        m_allPassed = false;
    }
    m_output.append(cond ? QStringLiteral("PASS ") + name
                         : QStringLiteral("FAIL ") + name + QStringLiteral(": ")
                               + (failMessage.isEmpty() ? QStringLiteral("condition false")
                                                        : failMessage));
}

QStringList SelfTest::runAll()
{
    m_output.clear();
    m_allPassed = true;

    testSettings();
    testEncodings();
    testDetection();
    testAsyncIo();
    testSyntax();
    testCompletion();
    return m_output;
}

void SelfTest::testSettings()
{
    AppSettings::init();
    const auto keys = {
        AppSettings::kEditorFontFamily,   AppSettings::kEditorFontSize,
        AppSettings::kTabWidth,           AppSettings::kAutoIndent,
        AppSettings::kColorEditorBg,      AppSettings::kCompletionEnabled,
        AppSettings::kEncodingDefaultOpen, AppSettings::kHexBytesPerRow,
        AppSettings::kFindMaxHighlights,
    };
    bool allDefaults = true;
    for (const char *key : keys) {
        if (!AppSettings::defaultValue(key).isValid())
            allDefaults = false;
    }
    check(allDefaults, "settings/defaults", "some keys lack defaults");

    check(AppSettings::instance()->value(AppSettings::kEditorFontSize).toInt() > 0,
          "settings/value-fallback", "fontSize fallback invalid");
}

void SelfTest::testEncodings()
{
    EncodingService::instance().init();

    struct Case
    {
        const char *id;
        QString text;
    };
    const QList<Case> cases = {
        {"UTF-8",      QStringLiteral("Hello 世界 café ñ")},
        {"UTF-16LE",   QStringLiteral("Hello 世界 café ñ")},
        {"UTF-16BE",   QStringLiteral("Hello 世界 café ñ")},
        {"UTF-32LE",   QStringLiteral("Hello 世界 café ñ")},
        {"UTF-32BE",   QStringLiteral("Hello 世界 café ñ")},
        {"ISO-8859-1", QStringLiteral("Hello café ñ")},
        {"GBK",        QStringLiteral("你好，世界")},
        {"Big5",       QStringLiteral("你好，世界")},
        {"Shift-JIS",  QStringLiteral("こんにちは世界")},
        {"EUC-KR",     QStringLiteral("안녕하세요")},
        {"KOI8-R",     QStringLiteral("Привет мир")},
        {"windows-1251", QStringLiteral("Привет мир")},
    };
    for (const Case &c : cases) {
        const QByteArray bytes = EncodingService::instance().encode(c.text, c.id, false);
        const bool round = EncodingService::instance().decode(bytes, c.id) == c.text;
        check(round, QStringLiteral("encoding/roundtrip-%1").arg(c.id),
              QStringLiteral("decode(encode(text)) != text"));
    }

    // BOM handling.
    const QByteArray bommed = EncodingService::instance().encode(
        QStringLiteral("hi"), QStringLiteral("UTF-8"), true);
    check(bommed.startsWith(QByteArray("\xEF\xBB\xBF", 3)), "encoding/bom-written");
    bool hadBom = false;
    const QString decoded = EncodingService::instance().decode(bommed, QStringLiteral("UTF-8"),
                                                               &hadBom);
    check(hadBom && decoded == QStringLiteral("hi"), "encoding/bom-stripped");

    // The catalogue must be much larger than the legacy 9 hard-coded ones.
    check(EncodingService::instance().encodings().size() >= 20,
          "encoding/catalogue", "fewer than 20 encodings registered");
}

void SelfTest::testDetection()
{
    auto &enc = EncodingService::instance();

    const QByteArray utf16 = enc.encode(QStringLiteral("hello detection"),
                                        QStringLiteral("UTF-16LE"), true);
    check(enc.detect(utf16, QStringLiteral("GBK")).id == QStringLiteral("UTF-16LE"),
          "detect/bom-utf16le");

    check(enc.detect(QByteArrayLiteral("plain ascii"), QStringLiteral("GBK")).id
              == QStringLiteral("UTF-8"),
          "detect/ascii-utf8");

    // GBK bytes are never valid UTF-8, so auto-detection must resolve them
    // to the configured legacy fallback.
    const QByteArray gbk = enc.encode(QStringLiteral("你好世界"), QStringLiteral("GBK"), false);
    check(!gbk.isEmpty(), "detect/gbk-encode", QString());
    const auto det = enc.detect(gbk, QStringLiteral("GBK"));
    check(det.id == QStringLiteral("GBK"), "detect/fallback-gbk",
          QStringLiteral("got %1 (input %2)").arg(det.id, QString::fromLatin1(gbk.toHex())));

    // Big5 bytes detected without any BOM.
    const QByteArray big5 = enc.encode(QStringLiteral("中文字"), QStringLiteral("Big5"), false);
    check(enc.detect(big5, QStringLiteral("Big5")).id == QStringLiteral("Big5"),
          "detect/fallback-big5");
}

void SelfTest::testAsyncIo()
{
    QTemporaryDir dir;
    check(dir.isValid(), "asyncio/tempdir");
    if (!dir.isValid())
        return;

    const QString path = dir.filePath("sample.txt");
    const QString payload = QStringLiteral("line one\nline two\n你好\n");
    {
        QTemporaryFile probe; // ensure dir exists & writable
        Q_UNUSED(probe);
    }
    QFile f(path);
    check(f.open(QIODevice::WriteOnly | QIODevice::Truncate), "asyncio/write-setup");
    f.write(EncodingService::instance().encode(payload, QStringLiteral("UTF-8"), false));
    f.close();

    QEventLoop loop;
    int progressCalls = 0;
    bool onMainThread = false;
    TextLoadResult result;
    loadTextFileAsync(
        path, EncodingService::AUTO, &loop,
        [&](TextLoadResult r) {
            result = r;
            onMainThread = QThread::currentThread() == loop.thread();
            loop.quit();
        },
        [&](qint64, qint64) { ++progressCalls; });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    check(result.ok, "asyncio/load-ok", result.error);
    check(result.text == payload, "asyncio/load-content");
    check(result.encoding == QStringLiteral("UTF-8"), "asyncio/load-encoding",
          result.encoding);
    check(onMainThread, "asyncio/callback-thread", "callback not on the GUI thread");
    check(progressCalls >= 1, "asyncio/progress");

    bool saved = false;
    QString saveError;
    saveTextFileAsync(path, QStringLiteral("replaced"), QStringLiteral("UTF-8"), false,
                      &loop, [&](bool ok, QString err) {
                          saved = ok;
                          saveError = err;
                          loop.quit();
                      });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();
    check(saved, "asyncio/save", saveError);

    QFile check2(path);
    check2.open(QIODevice::ReadOnly);
    const QByteArray expect = EncodingService::instance().encode(
        QStringLiteral("replaced"), QStringLiteral("UTF-8"), false);
    check(check2.readAll() == expect, "asyncio/save-bytes");
    check2.close();
}

void SelfTest::testSyntax()
{
    SyntaxRepository::instance().init();
    auto def = SyntaxRepository::instance().definitionForFile(QStringLiteral("t.cpp"));
    check(def && def->name == QStringLiteral("C++"), "syntax/cpp-lookup",
          def ? def->name : QStringLiteral("null"));
    check(def && def->keywords.size() >= 10, "syntax/cpp-keywords");

    auto pyDef = SyntaxRepository::instance().definitionForFile(QStringLiteral("t.py"));
    check(pyDef && pyDef->name == QStringLiteral("Python"), "syntax/py-lookup",
          pyDef ? pyDef->name : QStringLiteral("null"));

    if (!def)
        return;

    // Multi-block comment state machine: block 1 enters the comment (100 =
    // InComment + 0), block 2 stays inside it, block 3 ends it back at 0.
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("int a; /* start\nstill comment\nmore */ int b;"));
    RuleSyntaxHighlighter hi(&doc, def);
    hi.rehighlight();
    const QTextBlock b1 = doc.firstBlock();
    const QTextBlock b2 = b1.next();
    const QTextBlock b3 = b2.next();
    check(b1.isValid() && b1.userState() == 100,
          "syntax/comment-open",
          QStringLiteral("userState=%1").arg(b1.userState()));
    check(b2.isValid() && b2.userState() == 100,
          "syntax/comment-inside",
          QStringLiteral("userState=%1").arg(b2.userState()));
    check(b3.isValid() && b3.userState() == 0,
          "syntax/comment-close",
          QStringLiteral("userState=%1").arg(b3.userState()));
}

void SelfTest::testCompletion()
{
    CompletionEngine::instance()->init();

    check(CompletionEngine::matchScore(QStringLiteral("QStringLiteral"),
                                       QStringLiteral("QString"), true)
              > CompletionEngine::matchScore(QStringLiteral("xQString"), QStringLiteral("QString"),
                                             true),
          "completion/prefix-beats-fuzzy");

    check(CompletionEngine::matchScore(QStringLiteral("QString"), QStringLiteral("qs"), true) > 0,
          "completion/fuzzy-match");
    check(CompletionEngine::matchScore(QStringLiteral("QString"), QStringLiteral("zs"), true) < 0,
          "completion/fuzzy-reject");

    const QList<CompletionCandidate> kw =
        CompletionEngine::instance()->compute(QStringLiteral("clas"),
                                             QStringLiteral("t.cpp"), nullptr);
    check(std::any_of(kw.cbegin(), kw.cend(),
                      [](const CompletionCandidate &c) {
                          return c.text == QStringLiteral("class") && c.kind == 0;
                      }),
          "completion/keyword-class");

    const QSet<QString> docWords = {QStringLiteral("myVariableName"),
                                    QStringLiteral("myopia")};
    const QList<CompletionCandidate> dw = CompletionEngine::instance()->compute(
        QStringLiteral("myVa"), QStringLiteral("t.cpp"), &docWords);
    check(!dw.isEmpty() && dw.first().text == QStringLiteral("myVariableName"),
          "completion/docword-rank");

    const QList<Snippet> snips =
        CompletionEngine::instance()->snippetsFor(QStringLiteral("t.cpp"));
    check(!snips.isEmpty(), "completion/snippets-loaded");

    const QSet<QString> words = CompletionEngine::extractWords(
        QStringLiteral("alpha beta123 42 gamma"), 100);
    check(words.contains(QStringLiteral("alpha")) && words.contains(QStringLiteral("beta123"))
              && !words.contains(QStringLiteral("42")),
          "completion/extract-words");
}

} // namespace AppleCat::Core
