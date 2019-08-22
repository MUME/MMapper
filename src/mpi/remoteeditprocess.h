#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

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
    ~RemoteEditProcess();

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

    QProcess m_process{};
    QTemporaryFile m_file{};
    QString m_newBody{};
    QDateTime m_previousTime{};
};
