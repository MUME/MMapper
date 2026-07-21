#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/ConfigEnums.h"
#include "../global/macros.h"

#include <vector>

#include <QDate>
#include <QDateTime>
#include <QFileInfo>
#include <QString>

// Pure, filesystem-free log cleanup policy so that it can be unit tested.
namespace log_cleanup {

struct NODISCARD LogFile final
{
    QString path;
    QDateTime time;
    qint64 size = 0;
};

struct NODISCARD Policy final
{
    AutoLoggerEnum strategy = AutoLoggerEnum::KeepForever;
    int deleteWhenLogsReachDays = 0;
    qint64 deleteWhenLogsReachBytes = 0;
};

// Sandboxed environments (e.g. Flatpak) and some filesystems report a
// birth time of 0 (1970-01-01) or none at all; fall back to lastModified()
// so such logs are not mistaken for decades-old files.
NODISCARD QDateTime getFileTime(const QFileInfo &fileInfo);

// Returns the subset of `files` that the policy says should be deleted,
// ordered newest to oldest. DeleteSize keeps the newest logs whose cumulative
// size stays under the limit and deletes everything older.
NODISCARD std::vector<LogFile> selectLogsToDelete(std::vector<LogFile> files,
                                                  const Policy &policy,
                                                  const QDate &today);

} // namespace log_cleanup
