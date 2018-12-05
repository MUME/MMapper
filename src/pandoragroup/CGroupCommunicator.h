#pragma once
/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef CGROUPCOMMUNICATOR_H_
#define CGROUPCOMMUNICATOR_H_

#include <cstdint>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtCore>

#include "GroupSocket.h"
#include "groupaction.h"
#include "mmapper2group.h"

class CGroup;
class GroupAuthority;

class CGroupCommunicator : public QObject
{
    Q_OBJECT
public:
    explicit CGroupCommunicator(GroupManagerState mode, Mmapper2Group *parent);

    static constexpr const ProtocolVersion PROTOCOL_VERSION_103 = 103;
    static constexpr const ProtocolVersion PROTOCOL_VERSION_102 = 102;

    // TODO: password and encryption options
    enum class Messages {
        NONE, // Unused
        ACK,
        REQ_LOGIN,
        REQ_ACK,
        REQ_HANDSHAKE,
        REQ_INFO,
        PROT_VERSION, // Unused
        GTELL,
        STATE_LOGGED,
        STATE_KICKED,
        ADD_CHAR,
        REMOVE_CHAR,
        UPDATE_CHAR,
        RENAME_CHAR
    };

    GroupManagerState getMode() const { return mode; }
    void sendSelfRename(const QByteArray &, const QByteArray &);

    virtual void stop() = 0;
    virtual bool start() = 0;
    virtual void sendCharUpdate(const QVariantMap &map) = 0;
    virtual bool kickCharacter(const QByteArray &) = 0;

protected:
    void sendCharUpdate(GroupSocket *, const QVariantMap &);
    void sendMessage(GroupSocket *, const Messages, const QByteArray & = "");
    void sendMessage(GroupSocket *, const Messages, const QVariantMap &);

    virtual void sendGroupTellMessage(const QVariantMap &map) = 0;
    virtual void sendCharRename(const QVariantMap &map) = 0;

    QByteArray formMessageBlock(const Messages message, const QVariantMap &data);
    CGroup *getGroup();
    GroupAuthority *getAuthority();

public slots:
    void incomingData(GroupSocket *, const QByteArray &);
    void sendGroupTell(const QByteArray &);
    void relayLog(const QString &);
    virtual void retrieveData(GroupSocket *, const Messages, const QVariantMap &) = 0;
    virtual void connectionClosed(GroupSocket *) = 0;

signals:
    void messageBox(QString message);
    void scheduleAction(GroupAction *action);
    void gTellArrived(QVariantMap node);
    void sendLog(const QString &);

private:
    GroupManagerState mode = GroupManagerState::Off;
};

#endif /*CGROUPCOMMUNICATOR_H_*/
