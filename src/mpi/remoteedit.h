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

#include <climits>
#include <map>
#include <memory>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>
#include <QtGlobal>

#include "remoteeditsession.h"

class RemoteEditSession;

class RemoteEdit : public QObject
{
    Q_OBJECT
    friend class RemoteEditSession;

public:
    explicit RemoteEdit(QObject *parent = nullptr)
        : QObject(parent)
    {}
    ~RemoteEdit() = default;

public slots:
    void remoteView(const QString &, const QString &);
    void remoteEdit(int, const QString &, const QString &);

signals:
    void sendToSocket(const QByteArray &);

protected:
    void cancel(const RemoteEditSession *);
    void save(const RemoteEditSession *);

private:
    uint getSessionCount() { return greatestUsedId == UINT_MAX ? 0 : greatestUsedId + 1; }
    void addSession(int, const QString &, QString);
    void removeSession(uint);

    uint greatestUsedId{0};
    std::map<uint, std::unique_ptr<RemoteEditSession>> m_sessions;
};
