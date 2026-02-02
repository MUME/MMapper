#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Mattias 'Mew_' Viklund <devmew@exedump.com> (Mirnir)

#include "../global/macros.h"

#include <fstream>
#include <functional>
#include <string>

#include <QFileInfoList>
#include <QObject>

class NODISCARD_QOBJECT AutoLogger final : public QObject
{
    Q_OBJECT

private:
    const std::string m_runId;
    std::fstream m_logFile;
    int m_curBytes = 0;
    int m_curFile = 0;
    bool m_shouldLog = true;

public:
    explicit AutoLogger(QObject *parent);
    ~AutoLogger() final;

private:
    NODISCARD bool writeLine(const QString &str);
    void deleteOldLogs();
    static void deleteLogs(const QFileInfoList &files);
    void showDeleteDialog(QString message, std::function<void(bool)> callback);
    NODISCARD bool createFile();

public slots:
    void slot_writeToLog(const QString &str);
    void slot_shouldLog(bool echo);
    void slot_onConnected();
};
