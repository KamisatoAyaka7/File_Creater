#include "appsettings.h"
#include "coreinit.h"
#include "mainwindow.h"
#include "selftest.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Allow --selftest output to reach the launching console.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("szy"));
    QApplication::setApplicationName(QStringLiteral("Apple_Cat"));
    QApplication::setApplicationVersion(QString::fromLatin1(AppleCat::Core::coreVersion()));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Apple_Cat — tabbed text editor with async loading, "
                       "hex editing and plugins."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption selfTestOption(QStringLiteral("selftest"),
                                      QStringLiteral("Run headless self checks and exit."));
    parser.addOption(selfTestOption);
    parser.addPositionalArgument(QStringLiteral("file"),
                                 QStringLiteral("File(s) to open."), QStringLiteral("[file...]"));
    parser.process(app);

    AppleCat::Core::coreInit();

    if (parser.isSet(selfTestOption)) {
        AppleCat::Core::SelfTest test;
        const QStringList lines = test.runAll();
        QString output;
        QTextStream out(&output);
        for (const QString &line : lines)
            out << line << '\n';
        out << (test.allPassed() ? QStringLiteral("ALL TESTS PASSED")
                                 : QStringLiteral("TESTS FAILED"))
            << '\n';
        out.flush();

        // GUI-subsystem executables have no guaranteed stdout; persist the
        // report so `AppleCat --selftest` is scriptable.
        QFile report(QCoreApplication::applicationDirPath()
                     + QStringLiteral("/selftest_result.txt"));
        if (report.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            report.write(output.toUtf8());
            report.close();
        }

        QTextStream console(stdout);
        console << output;
        console.flush();
        return test.allPassed() ? 0 : 1;
    }

    AppleCat::Gui::MainWindow mainWindow;
    mainWindow.show();

    const QStringList files = parser.positionalArguments();
    if (files.isEmpty()) {
        mainWindow.startNew();
    } else {
        for (const QString &f : files)
            mainWindow.startOpen(f);
    }

    return app.exec();
}
