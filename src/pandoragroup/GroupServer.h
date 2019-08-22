#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupCommunicator.h"
#include "GroupPortMapper.h"

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
    bool start() override;
    void stop() override;
    void sendCharUpdate(const QVariantMap &map) override;
    void sendCharRename(const QVariantMap &map) override;
    bool kickCharacter(const QByteArray &) override;

private:
    void parseHandshake(GroupSocket *socket, const QVariantMap &data);
    void parseLoginInformation(GroupSocket *socket, const QVariantMap &data);
    void sendGroupInformation(GroupSocket *socket);
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
    GroupPortMapper portMapper;
};
