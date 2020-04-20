#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Mattias 'Mew_' Viklund <devmew@exedump.com> (Mirnir)

#include <fstream>
#include <string>
#include <QFileInfoList>
#include <QObject>

class MainWindow;

class AutoLogger : public QObject
{
    Q_OBJECT
public:
    explicit AutoLogger(QObject *parent);
    ~AutoLogger() override;

public slots:
    void writeToLog(const QByteArray &ba);
    void shouldLog(bool echo);
    void onConnected();

private:
    bool writeLine(const QByteArray &ba);
    void deleteOldLogs();
    void deleteLogs(const QFileInfoList &files);
    bool showDeleteDialog(QString message);

    bool createFile();

private:
    const std::string m_runId;
    std::fstream m_logFile;
    int m_curBytes = 0;
    int m_curFile = 0;
    bool m_shouldLog = true;
};
