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

#ifndef GROUPSERVER_H_
#define GROUPSERVER_H_

#include "CGroupCommunicator.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QTcpServer>
#include <QVariantMap>
#include <QtCore>
#include <QtGlobal>

class GroupServer;
class GroupSocket;
class GroupAuthority;
class QObject;

class GroupTcpServer final : public QTcpServer
{
    Q_OBJECT

public:
    explicit GroupTcpServer(GroupServer *parent);
    virtual ~GroupTcpServer() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void signal_incomingConnection(qintptr socketDescriptor);
};

class GroupServer final : public CGroupCommunicator
{
    Q_OBJECT

public:
    explicit GroupServer(Mmapper2Group *parent);
    ~GroupServer() override;

protected slots:
    void relayMessage(GroupSocket *socket, const Messages message, const QVariantMap &data);
    void connectionEstablished(GroupSocket *socket);
    void onRevokeWhitelist(const QByteArray &secret);
    void retrieveData(GroupSocket *socket, const Messages message, const QVariantMap &data) override;
    void connectionClosed(GroupSocket *socket) override;

protected:
    void sendRemoveUserNotification(GroupSocket *socket, const QByteArray &name);
    void sendGroupTellMessage(const QVariantMap &root) override;
    void start() override;
    void stop() override;
    void sendCharUpdate(const QVariantMap &map) override;
    void sendCharRename(const QVariantMap &map) override;
    bool kickCharacter(const QByteArray &) override;

private:
    void parseHandshake(GroupSocket *socket, const QVariantMap &data);
    void parseLoginInformation(GroupSocket *socket, const QVariantMap &data);
    void sendGroupInformation(GroupSocket *socket);
    void serverStartupFailed();
    void kickConnection(GroupSocket *socket, const QString &message);

protected slots:
    void onIncomingConnection(qintptr socketDescriptor);
    void errorInConnection(GroupSocket *, const QString &);

private:
    void sendToAll(const QByteArray &);
    void sendToAllExceptOne(GroupSocket *exception, const QByteArray &);
    void closeAll();
    void closeOne(GroupSocket *target);
    void connectAll(GroupSocket *);
    void disconnectAll(GroupSocket *);

    QList<GroupSocket *> clientsList{};
    GroupTcpServer server;
};

#endif /*GROUPSERVER_H_*/
