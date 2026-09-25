// Headless self checks, run with `AppleCat --selftest`. Exercises the core
// services (settings, encodings, async IO, syntax, completion) and prints
// one PASS/FAIL line per test.

#pragma once

#include <QString>
#include <QStringList>

namespace AppleCat::Core {

class SelfTest
{
public:
    // Runs all tests; returns the output lines ("PASS name" / "FAIL name: reason").
    QStringList runAll();
    bool allPassed() const { return m_allPassed; }

private:
    void check(bool cond, const QString &name, const QString &failMessage = QString());
    void testSettings();
    void testEncodings();
    void testDetection();
    void testAsyncIo();
    void testSyntax();
    void testCompletion();

    bool m_allPassed = true;
    QStringList m_output;
};

} // namespace AppleCat::Core
