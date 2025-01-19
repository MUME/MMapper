#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "remoteeditsession.h"

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QtCore>

class NODISCARD_QOBJECT RemoteEditProcess final : public QObject
{
    Q_OBJECT

public:
    explicit RemoteEditProcess(bool editSession,
                               const QString &title,
                               const QString &body,
                               QObject *parent);
    ~RemoteEditProcess() final;

private:
    virtual void virt_onError(QProcess::ProcessError);
    virtual void virt_onFinished(int, QProcess::ExitStatus);

protected slots:
    void slot_onError(QProcess::ProcessError err) { virt_onError(err); }
    void slot_onFinished(int exitCode, QProcess::ExitStatus status)
    {
        virt_onFinished(exitCode, status);
    }

signals:
    void sig_cancel();
    void sig_save(const QString &);

private:
    NODISCARD static QStringList splitCommandLine(const QString &cmdLine);

    const QString m_title;
    const QString m_body;
    const bool m_editSession;

    QProcess m_process;
    QString m_fileName;
    QDateTime m_previousTime;
};
