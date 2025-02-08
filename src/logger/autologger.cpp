// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Mattias 'Mew_' Viklund <devmew@exedump.com> (Mirnir)

#include "autologger.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/random.h"

#include <algorithm>
#include <sstream>
#include <tuple>

#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>

NODISCARD static std::string generateRunId()
{
    // Generate 6 random characters
    std::ostringstream os;
    for (int i = 0; i < 6; ++i) {
        os << static_cast<char>(getRandom(25u) + 65u);
    }
    return os.str();
}

AutoLogger::AutoLogger(QObject *const parent)
    : QObject(parent)
    , m_runId{generateRunId()}
{}

AutoLogger::~AutoLogger()
{
    m_logFile.flush();
    m_logFile.close();
}

bool AutoLogger::createFile()
{
    if (m_logFile.is_open()) {
        m_logFile.close();
    }

    const auto &settings = getConfig().autoLog;

    const auto &path = settings.autoLogDirectory;
    QDir dir;
    if (dir.mkpath(path)) {
        dir.setPath(path);
    } else {
        return false;
    }

    QString fileName = QString("MMapper_Log_%1_%2_%3.txt")
                           .arg(QDate::currentDate().toString("yyyy_MM_dd"))
                           .arg(QString::number(m_curFile))
                           .arg(mmqt::toQStringUtf8(m_runId));
    m_logFile.open(mmqt::toStdStringUtf8(dir.absoluteFilePath(fileName)),
                   std::fstream::out | std::fstream::binary | std::fstream::app);
    if (!m_logFile.is_open()) { // Could not create file.
        return false;
    }

    m_curBytes = 0;
    ++m_curFile;

    return true;
}

bool AutoLogger::writeLine(const QString &str)
{
    if (!m_shouldLog || !getConfig().autoLog.autoLog) {
        return false;
    }

    bool created = true;
    if (!m_logFile.is_open()) {
        created = createFile();

    } else if (m_curBytes > getConfig().autoLog.rotateWhenLogsReachBytes) {
        m_logFile.close();
        created = createFile();
    }
    if (!created) {
        setConfig().autoLog.autoLog = false;
        QMessageBox::warning(checked_dynamic_downcast<QWidget *>(parent()), // MainWindow
                             "MMapper AutoLogger",
                             "Unable to create log file.\n\nLogging has been disabled.");
        return false;
    }

    // ANSI marks removed upstream by GameObserver
    const auto &line = mmqt::toStdStringUtf8(str);

    m_logFile << line;
    m_logFile.flush();
    m_curBytes += static_cast<int>(line.length());

    return true;
}

void AutoLogger::deleteOldLogs()
{
    auto &conf = getConfig().autoLog;

    if (conf.cleanupStrategy == AutoLoggerEnum::KeepForever) {
        return;
    }

    auto fileInfoList = QDir(conf.autoLogDirectory)
                            .entryInfoList(QStringList("MMapper_Log_*.txt"), QDir::Files);
    if (fileInfoList.empty()) {
        return;
    }

    // Sort files so we can delete the oldest
    std::sort(fileInfoList.begin(), fileInfoList.end(), [](const auto &a, const auto &b) {
        return a.birthTime() < b.birthTime();
    });

    qint64 totalFileSize = 0, deleteFileSize = 0;
    QList<QFileInfo> filesToDelete;
    const QDate &today = QDate::currentDate();
    for (const auto &fileInfo : fileInfoList) {
        totalFileSize += fileInfo.size();
        bool deleteFile = false;
        switch (conf.cleanupStrategy) {
        case AutoLoggerEnum::DeleteDays:
            if (fileInfo.birthTime().date().daysTo(today) >= conf.deleteWhenLogsReachDays) {
                deleteFile = true;
            }
            break;
        case AutoLoggerEnum::DeleteSize:
            if (totalFileSize >= conf.deleteWhenLogsReachBytes) {
                deleteFile = true;
            }
            break;
        case AutoLoggerEnum::KeepForever:
            break;
        default:
            abort();
        }
        if (deleteFile) {
            deleteFileSize += fileInfo.size();
            filesToDelete.append(fileInfo);
        }
    }

    if (filesToDelete.empty()) {
        return;
    }

    if (conf.askDelete) {
        QString unit = "KB";
        QStringList list = {"MB", "GB", "TB"};
        QStringListIterator it(list);
        auto num = static_cast<double>(deleteFileSize / 1024);
        while (num > 1024.0 && it.hasNext()) {
            unit = it.next();
            num /= 1024.0;
        }
        if (!showDeleteDialog(QString("There are %1 %2 of old logs.\n\nDo you want to delete them?")
                                  .arg(QString::number(num, 'f', 1))
                                  .arg(unit))) {
            return;
        }
    }

    QFileInfoList fileList(filesToDelete);
    deleteLogs(fileList);
}

void AutoLogger::deleteLogs(const QFileInfoList &files)
{
    for (const auto &fileInfo : files) {
        QString filepath = fileInfo.absoluteFilePath();
        QDir deletefile;
        deletefile.setPath(filepath);
        deletefile.remove(filepath);
        qDebug() << "Deleted log " + filepath + ".";
    }
}

bool AutoLogger::showDeleteDialog(QString message)
{
    QMessageBox msgBox(checked_dynamic_downcast<QWidget *>(parent())); // MainWindow
    msgBox.setText(message);
    msgBox.setWindowTitle("MMapper AutoLogger");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::No);

    int result = msgBox.exec();
    return result == QMessageBox::Yes;
}

void AutoLogger::slot_writeToLog(const QString &str)
{
    std::ignore = writeLine(str);
}

void AutoLogger::slot_shouldLog(bool echo)
{
    m_shouldLog = echo;
}

void AutoLogger::slot_onConnected()
{
    if (getConfig().autoLog.cleanupStrategy != AutoLoggerEnum::KeepForever) {
        deleteOldLogs();
    }

    if (getConfig().autoLog.autoLog) {
        if (!createFile()) {
            qWarning() << "Unable to create log file for autologger.";
        }
    }
}
