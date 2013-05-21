/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTest>
#include <QSet>
#include <QProcess>
#include <QDebug>
#include <QLibraryInfo>

enum FindSubdirsMode {
    Flat = 0,
    Recursive
};

class tst_MakeTestSelfTest: public QObject
{
    Q_OBJECT

private slots:
    void tests_auto_pro();

    void tests_pro_files();
    void tests_pro_files_data();

    void naming_convention();
    void naming_convention_data();

private:
    QStringList find_subdirs(QString const&, FindSubdirsMode, QString const& = QString());

    QSet<QString> all_test_classes;
};

bool looks_like_testcase(QString const&,QString*);
bool looks_like_subdirs(QString const&);
QStringList find_test_class(QString const&);

/*
    Verify that auto.pro only contains other .pro files (and not directories).
    We enforce this so that we can process every .pro file other than auto.pro
    independently and get all the tests.
    If tests were allowed to appear directly in auto.pro, we'd have the problem
    that we need to somehow run these tests from auto.pro while preventing
    recursion into the other .pro files.
*/
void tst_MakeTestSelfTest::tests_auto_pro()
{
    QStringList subdirsList = find_subdirs(SRCDIR "/../auto.pro", Flat);
    if (QTest::currentTestFailed()) {
        return;
    }

    foreach (QString const& subdir, subdirsList) {
        QVERIFY2(subdir.endsWith(".pro"), qPrintable(QString(
            "auto.pro contains a subdir `%1'.\n"
            "auto.pro must _only_ contain other .pro files, not actual subdirs.\n"
            "Please move `%1' into some other .pro file referenced by auto.pro."
        ).arg(subdir)));
    }
}

/* Verify that all tests are listed somewhere in one of the autotest .pro files */
void tst_MakeTestSelfTest::tests_pro_files()
{
    static QStringList lines;

    if (lines.isEmpty()) {
        QDir dir(SRCDIR "/..");
        QStringList proFiles = dir.entryList(QStringList() << "*.pro");
        foreach (QString const& proFile, proFiles) {
            QString filename = QString("%1/../%2").arg(SRCDIR).arg(proFile);
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly)) {
                QFAIL(qPrintable(QString("open %1: %2").arg(filename).arg(file.errorString())));
            }
            while (!file.atEnd()) {
                lines << file.readLine().trimmed();
            }
        }
    }

    QFETCH(QString, subdir);
    QRegExp re(QString("( |=|^|#)%1( |\\\\|$)").arg(QRegExp::escape(subdir)));
    foreach (const QString& line, lines) {
        if (re.indexIn(line) != -1) {
            return;
        }
    }



    QFAIL(qPrintable(QString(
        "Subdir `%1' is missing from tests/auto/*.pro\n"
        "This means the test won't be compiled or run on any platform.\n"
        "If this is intentional, please put the test name in a comment in one of the .pro files.").arg(subdir))
    );

}

void tst_MakeTestSelfTest::tests_pro_files_data()
{
    QTest::addColumn<QString>("subdir");
    QDir dir(SRCDIR "/..");
    QStringList subdirs = dir.entryList(QDir::AllDirs|QDir::NoDotAndDotDot);

    foreach (const QString& subdir, subdirs) {
        if (subdir == QString::fromLatin1("tmp")
            || subdir.startsWith("."))
        {
            continue;
        }
        QTest::newRow(qPrintable(subdir)) << subdir;
    }
}

QString format_list(QStringList const& list)
{
    if (list.count() == 1) {
        return list.at(0);
    }
    return QString("one of (%1)").arg(list.join(", "));
}

void tst_MakeTestSelfTest::naming_convention()
{
    QFETCH(QString, subdir);
    QFETCH(QString, target);

    QDir dir(SRCDIR "/../" + subdir);

    QStringList cppfiles = dir.entryList(QStringList() << "*.h" << "*.cpp");
    if (cppfiles.isEmpty()) {
        // Common convention is to have test/test.pro and source files in parent dir
        if (dir.dirName() == "test") {
            dir.cdUp();
            cppfiles = dir.entryList(QStringList() << "*.h" << "*.cpp");
        }

        if (cppfiles.isEmpty()) {
            QSKIP("Couldn't locate source files for test", SkipSingle);
        }
    }

    QStringList possible_test_classes;
    foreach (QString const& file, cppfiles) {
        possible_test_classes << find_test_class(dir.path() + "/" + file);
    }

    if (possible_test_classes.isEmpty()) {
        QSKIP(qPrintable(QString("Couldn't locate test class in %1").arg(format_list(cppfiles))), SkipSingle);
    }

    QVERIFY2(possible_test_classes.contains(target), qPrintable(QString(
        "TARGET is %1, while test class appears to be %2.\n"
        "TARGET and test class _must_ match so that all testcase names can be accurately "
        "determined even if a test fails to compile or run.")
        .arg(target)
        .arg(format_list(possible_test_classes))
    ));

    QVERIFY2(!all_test_classes.contains(target), qPrintable(QString(
        "It looks like there are multiple tests named %1.\n"
        "This makes it impossible to separate results for these tests.\n"
        "Please ensure all tests are uniquely named.")
        .arg(target)
    ));

    all_test_classes << target;
}

void tst_MakeTestSelfTest::naming_convention_data()
{
    QTest::addColumn<QString>("subdir");
    QTest::addColumn<QString>("target");

    foreach (const QString& subdir, find_subdirs(SRCDIR "/../auto.pro", Recursive)) {
        if (QFileInfo(SRCDIR "/../" + subdir).isDir()) {
            QString target;
            if (looks_like_testcase(SRCDIR "/../" + subdir + "/" + QFileInfo(subdir).baseName() + ".pro", &target)) {
                QTest::newRow(qPrintable(subdir)) << subdir << target.toLower();
            }
        }
    }
}

/*
    Returns true if a .pro file seems to be for an autotest.
    Running qmake to figure this out takes too long.
*/
bool looks_like_testcase(QString const& pro_file, QString* target)
{
    QFile file(pro_file);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    *target = QString();

    bool testcase = false;

    do {
        QByteArray line = file.readLine();
        if (line.isEmpty()) {
            break;
        }

        line = line.trimmed();
        line.replace(' ', "");

        if ((line.startsWith("CONFIG+=") || line.startsWith("CONFIG=")) && line.contains("testcase")) {
            testcase = true;
        }

        if (line.startsWith("TARGET=")) {
            *target = QString::fromLatin1(line.mid(sizeof("TARGET=")-1));
            if (target->contains('/')) {
                *target = target->right(target->lastIndexOf('/')+1);
            }
        }

        if (testcase && !target->isEmpty()) {
            break;
        }
    } while(1);

    if (!testcase) {
        return false;
    }

    if (!target->isEmpty() && !target->startsWith("tst_")) {
        return false;
    }

    // If no target was set, default to tst_<dirname>
    if (target->isEmpty()) {
        *target = "tst_" + QFileInfo(pro_file).baseName();
    }

    return true;
}

/*
    Returns true if a .pro file seems to be a subdirs project.
    Running qmake to figure this out takes too long.
*/
bool looks_like_subdirs(QString const& pro_file)
{
    QFile file(pro_file);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    do {
        QByteArray line = file.readLine();
        if (line.isEmpty()) {
            break;
        }

        line = line.trimmed();
        line.replace(' ', "");

        if (line == "TEMPLATE=subdirs") {
            return true;
        }
    } while(1);

    return false;
}

/*
    Returns a list of all subdirs in a given .pro file
*/
QStringList tst_MakeTestSelfTest::find_subdirs(QString const& pro_file, FindSubdirsMode mode, QString const& prefix)
{
    QStringList out;

    QByteArray features = qgetenv("QMAKEFEATURES");

    if (features.isEmpty()) {
        features = SRCDIR "/features";
    }
    else {
        features.prepend(SRCDIR "/features"
#ifdef Q_OS_WIN32
                ";"
#else
                ":"
#endif
        );
    }

    QStringList args;
    args << pro_file << "-o" << SRCDIR "/dummy_output" << "CONFIG+=dump_subdirs";

    /* Turn on every option there is, to ensure we process every single directory */
    args
        << "QT_CONFIG+=dbus"
        << "QT_CONFIG+=declarative"
        << "QT_CONFIG+=egl"
        << "QT_CONFIG+=multimedia"
        << "QT_CONFIG+=OdfWriter"
        << "QT_CONFIG+=opengl"
        << "QT_CONFIG+=openvg"
        << "QT_CONFIG+=phonon"
        << "QT_CONFIG+=private_tests"
        << "QT_CONFIG+=pulseaudio"
        << "QT_CONFIG+=qt3support"
        << "QT_CONFIG+=script"
        << "QT_CONFIG+=svg"
        << "QT_CONFIG+=webkit"
        << "QT_CONFIG+=xmlpatterns"
        << "CONFIG+=meego"
    ;



    QString cmd_with_args = QString("qmake %1").arg(args.join(" "));

    QProcess proc;

    proc.setProcessChannelMode(QProcess::MergedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QMAKEFEATURES", features);
    proc.setProcessEnvironment(env);

    QDir binDir(QLibraryInfo::location(QLibraryInfo::BinariesPath));
    proc.start(binDir.filePath("qmake"), args);
    if (!proc.waitForStarted(10000)) {
        QTest::qFail(qPrintable(QString("Failed to run qmake: %1\nCommand: %2")
            .arg(proc.errorString())
            .arg(cmd_with_args)),
            __FILE__, __LINE__
        );
        return out;
    }
    if (!proc.waitForFinished(30000)) {
        QTest::qFail(qPrintable(QString("qmake did not finish within 30 seconds\nCommand: %1\nOutput: %2")
            .arg(proc.errorString())
            .arg(cmd_with_args)
            .arg(QString::fromLocal8Bit(proc.readAll()))),
            __FILE__, __LINE__
        );
        return out;
    }

    if (proc.exitStatus() != QProcess::NormalExit) {
        QTest::qFail(qPrintable(QString("qmake crashed\nCommand: %1\nOutput: %2")
            .arg(cmd_with_args)
            .arg(QString::fromLocal8Bit(proc.readAll()))),
            __FILE__, __LINE__
        );
        return out;
    }

    if (proc.exitCode() != 0) {
        QTest::qFail(qPrintable(QString("qmake exited with code %1\nCommand: %2\nOutput: %3")
            .arg(proc.exitCode())
            .arg(cmd_with_args)
            .arg(QString::fromLocal8Bit(proc.readAll()))),
            __FILE__, __LINE__
        );
        return out;
    }

    QList<QByteArray> lines = proc.readAll().split('\n');
    if (!lines.count()) {
        QTest::qFail(qPrintable(QString("qmake seems to have not output anything\nCommand: %1\n")
            .arg(cmd_with_args)),
            __FILE__, __LINE__
        );
        return out;
    }

    foreach (QByteArray const& line, lines) {
        static const QByteArray marker = "Project MESSAGE: subdir: ";
        if (line.startsWith(marker)) {
            QString subdir = QString::fromLocal8Bit(line.mid(marker.size()).trimmed());
            out << prefix + subdir;

            if (mode == Flat) {
                continue;
            }

            // Need full path to subdir
            QString subdir_filepath = subdir;
            subdir_filepath.prepend(QFileInfo(pro_file).path() + "/");

            // Add subdirs recursively
            if (subdir.endsWith(".pro") && looks_like_subdirs(subdir_filepath)) {
                // Need full path to .pro file
                out << find_subdirs(subdir_filepath, mode, prefix);
            }

            if (QFileInfo(subdir_filepath).isDir()) {
                subdir_filepath += "/" + subdir + ".pro";
                if (looks_like_subdirs(subdir_filepath)) {
                    out << find_subdirs(subdir_filepath, mode, prefix + subdir + "/");
                }
            }
        }
    }

    return out;
}

QStringList find_test_class(QString const& filename)
{
    QStringList out;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return out;
    }

    static char const* klass_indicators[] = {
        "QTEST_MAIN(",
        "QTEST_APPLESS_MAIN(",
        "class",
        "staticconstcharklass[]=\"",  /* hax0r tests which define their own metaobject */
        0
    };

    do {
        QByteArray line = file.readLine();
        if (line.isEmpty()) {
            break;
        }

        line = line.trimmed();
        line.replace(' ', "");

        for (int i = 0; klass_indicators[i]; ++i) {
            char const* prefix = klass_indicators[i];
            if (!line.startsWith(prefix)) {
                continue;
            }
            QByteArray klass = line.mid(strlen(prefix));
            if (!klass.startsWith("tst_")) {
                continue;
            }
            for (int j = 0; j < klass.size(); ++j) {
                char c = klass[j];
                if (c == '_'
                        || (c >= '0' && c <= '9')
                        || (c >= 'A' && c <= 'Z')
                        || (c >= 'a' && c <= 'z')) {
                    continue;
                }
                else {
                    klass.truncate(j);
                    break;
                }
            }
            QString klass_str = QString::fromLocal8Bit(klass).toLower();
            if (!out.contains(klass_str))
                out << klass_str;
            break;
        }
    } while(1);

    return out;
}

QTEST_MAIN(tst_MakeTestSelfTest)
#include "tst_maketestselftest.moc"
