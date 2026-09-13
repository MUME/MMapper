// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestAutoLogger.h"

#include "../src/logger/LogCleanup.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using log_cleanup::LogFile;
using log_cleanup::Policy;

namespace {

QDateTime utc(int y, int m, int d)
{
    return QDateTime(QDate(y, m, d), QTime(0, 0, 0), QTimeZone::utc());
}

QStringList names(const std::vector<LogFile> &files)
{
    QStringList result;
    for (const auto &f : files) {
        result << f.path;
    }
    return result;
}

} // namespace

void TestAutoLogger::getFileTimeFallsBackToLastModified()
{
    // birthTime() cannot be mocked on a QFileInfo, so test the fallback
    // through a file we control: set mtime far in the past and make sure
    // getFileTime() never returns something older than that when birth time
    // is unavailable, and never returns the epoch.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("MMapper_Log_x.txt");
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x");
    }
    const QDateTime mtime = utc(2021, 6, 15);
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadWrite));
        QVERIFY(f.setFileTime(mtime, QFileDevice::FileModificationTime));
    }
    const QFileInfo info(path);
    const QDateTime t = log_cleanup::getFileTime(info);
    QVERIFY(t.isValid());
    QVERIFY(t.toMSecsSinceEpoch() > utc(1980, 1, 1).toMSecsSinceEpoch());
    // Either the real birth time (now-ish) or the mtime we set; never earlier.
    QVERIFY(t >= mtime);
    if (!info.birthTime().isValid() || info.birthTime().toMSecsSinceEpoch() <= 0) {
        QCOMPARE(t, info.lastModified());
    }
}

void TestAutoLogger::getFileTimeOnRealFile()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("test log data");
    tempFile.flush();

    const QDateTime t = log_cleanup::getFileTime(QFileInfo(tempFile.fileName()));
    QVERIFY(t.isValid());
    QVERIFY(t.secsTo(QDateTime::currentDateTime()) < 60);
}

void TestAutoLogger::keepForeverSelectsNothing()
{
    std::vector<LogFile> files = {{"a", utc(2000, 1, 1), 1000}, {"b", utc(2001, 1, 1), 1000}};
    const Policy policy{AutoLoggerEnum::KeepForever, 0, 0};
    QVERIFY(log_cleanup::selectLogsToDelete(files, policy, QDate(2025, 1, 10)).empty());
}

void TestAutoLogger::deleteDaysUsesFallbackTime()
{
    const QDate today(2025, 1, 10);
    const Policy policy{AutoLoggerEnum::DeleteDays, 3, 0};

    std::vector<LogFile> files = {
        {"nine_days_old", utc(2025, 1, 1), 100},
        {"exactly_three_days_old", utc(2025, 1, 7), 100},
        {"two_days_old", utc(2025, 1, 8), 100},
        {"one_day_old", utc(2025, 1, 9), 100},
    };

    const auto deleted = log_cleanup::selectLogsToDelete(files, policy, today);
    // Ordered newest to oldest.
    QCOMPARE(names(deleted), QStringList({"exactly_three_days_old", "nine_days_old"}));
}

void TestAutoLogger::deleteSizeKeepsNewest()
{
    const QDate today(2025, 1, 10);
    // Deliberately unsorted input; sizes: newest 30, middle 50, oldest 60.
    const std::vector<LogFile> files = {
        {"oldest", utc(2023, 5, 10), 60},
        {"newest", utc(2025, 5, 10), 30},
        {"middle", utc(2024, 5, 10), 50},
    };

    // Limit 100: newest (30) + middle (80) stay; oldest pushes total to 140.
    QCOMPARE(names(log_cleanup::selectLogsToDelete(files,
                                                   {AutoLoggerEnum::DeleteSize, 0, 100},
                                                   today)),
             QStringList({"oldest"}));

    // Limit exactly equal to the total: "after 140" is not reached, nothing deleted.
    QVERIFY(
        log_cleanup::selectLogsToDelete(files, {AutoLoggerEnum::DeleteSize, 0, 140}, today).empty());

    // One byte less than the total: the oldest crosses the line.
    QCOMPARE(names(log_cleanup::selectLogsToDelete(files,
                                                   {AutoLoggerEnum::DeleteSize, 0, 139},
                                                   today)),
             QStringList({"oldest"}));

    // Limit 60: only the newest survives.
    QCOMPARE(names(
                 log_cleanup::selectLogsToDelete(files, {AutoLoggerEnum::DeleteSize, 0, 60}, today)),
             QStringList({"middle", "oldest"}));

    // Generous limit: nothing deleted.
    QVERIFY(log_cleanup::selectLogsToDelete(files, {AutoLoggerEnum::DeleteSize, 0, 1000}, today)
                .empty());
}

void TestAutoLogger::deleteSizeSingleOversizedFile()
{
    // The newest log is never deleted, even when it alone exceeds the limit;
    // only the older logs are pruned.
    const std::vector<LogFile> files = {
        {"newest_huge", utc(2025, 5, 10), 500},
        {"older_small", utc(2024, 5, 10), 10},
    };
    QCOMPARE(names(log_cleanup::selectLogsToDelete(files,
                                                   {AutoLoggerEnum::DeleteSize, 0, 100},
                                                   QDate(2025, 6, 1))),
             QStringList({"older_small"}));

    // A lone log is never deleted by the size strategy.
    const std::vector<LogFile> single = {{"only", utc(2025, 5, 10), 500}};
    QVERIFY(log_cleanup::selectLogsToDelete(single,
                                            {AutoLoggerEnum::DeleteSize, 0, 100},
                                            QDate(2025, 6, 1))
                .empty());
}

QTEST_MAIN(TestAutoLogger)
