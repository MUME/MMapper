// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "LogCleanup.h"

#include <algorithm>
#include <cstdlib>

namespace log_cleanup {

static constexpr qint64 EPOCH_CUTOFF_MS = 315532800000LL; // 1980-01-01 00:00:00 UTC

QDateTime getFileTime(const QFileInfo &fileInfo)
{
    if (const QDateTime birth = fileInfo.birthTime();
        birth.isValid() && birth.toMSecsSinceEpoch() > EPOCH_CUTOFF_MS) {
        return birth;
    }
    return fileInfo.lastModified();
}

std::vector<LogFile> selectLogsToDelete(std::vector<LogFile> files,
                                        const Policy &policy,
                                        const QDate &today)
{
    std::vector<LogFile> filesToDelete;
    if (policy.strategy == AutoLoggerEnum::KeepForever) {
        return filesToDelete;
    }

    // Newest first, so that the size strategy retains recent logs.
    std::stable_sort(files.begin(), files.end(), [](const LogFile &a, const LogFile &b) {
        return a.time > b.time;
    });

    qint64 totalFileSize = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        auto &file = files[i];
        totalFileSize += file.size;
        bool deleteFile = false;
        switch (policy.strategy) {
        case AutoLoggerEnum::DeleteDays:
            // "Delete logs after N days": gone once N days have elapsed.
            deleteFile = file.time.date().daysTo(today) >= policy.deleteWhenLogsReachDays;
            break;
        case AutoLoggerEnum::DeleteSize:
            // "Delete logs after N MBs": prune the oldest logs once the total
            // exceeds N. The newest log is always kept, even if it alone is
            // over the limit, so the budget never wipes out every log.
            deleteFile = i > 0 && totalFileSize > policy.deleteWhenLogsReachBytes;
            break;
        case AutoLoggerEnum::KeepForever:
            break;
        default:
            std::abort();
        }
        if (deleteFile) {
            filesToDelete.push_back(std::move(file));
        }
    }
    return filesToDelete;
}

} // namespace log_cleanup
