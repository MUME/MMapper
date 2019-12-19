#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QtCore>

#include "remoteeditsession.h"

class RemoteEditProcess : public QObject
{
    Q_OBJECT

public:
    explicit RemoteEditProcess(bool editSession,
                               const QString &title,
                               const QString &body,
                               QObject *parent = nullptr);
    ~RemoteEditProcess() override;

protected slots:
    virtual void onError(QProcess::ProcessError);
    virtual void onFinished(int, QProcess::ExitStatus);

signals:
    void cancel();
    void save(const QString &);

private:
    QStringList splitCommandLine(const QString &cmdLine);

    const QString m_title;
    const QString m_body;
    const bool m_editSession;

    QProcess m_process;
    QTemporaryFile m_file;
    QString m_fileName;
    QDateTime m_previousTime;
};
