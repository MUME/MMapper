#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupCommunicator.h"
#include "GroupPortMapper.h"

#include <vector>
#include <QByteArray>
#include <QPointer>
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
    ~GroupTcpServer() final;

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
    ~GroupServer() final;

protected slots:
    void slot_relayMessage(GroupSocket *socket, MessagesEnum message, const QVariantMap &data);
    void slot_connectionEstablished(GroupSocket *socket);
    void slot_onRevokeWhitelist(const QByteArray &secret);
    void slot_onIncomingConnection(qintptr socketDescriptor);
    void slot_errorInConnection(GroupSocket *, const QString &);

protected:
    void sendRemoveUserNotification(GroupSocket *socket, const QByteArray &name);

private:
    NODISCARD bool virt_start() final;
    void virt_stop() final;

private:
    void virt_connectionClosed(GroupSocket *socket) final;
    void virt_kickCharacter(const QByteArray &) final;
    void virt_retrieveData(GroupSocket *socket, MessagesEnum message, const QVariantMap &data) final;
    void virt_sendCharRename(const QVariantMap &map) final;
    void virt_sendCharUpdate(const QVariantMap &map) final;
    void virt_sendGroupTellMessage(const QVariantMap &root) final;

private:
    void parseHandshake(GroupSocket *socket, const QVariantMap &data);
    void parseLoginInformation(GroupSocket *socket, const QVariantMap &data);
    void sendGroupInformation(GroupSocket *socket);
    void kickConnection(GroupSocket *socket, const QString &message);

private:
    void sendToAll(const QByteArray &);
    void sendToAllExceptOne(GroupSocket *exception, const QByteArray &);
    void closeAll();
    void closeOne(GroupSocket *target);
    void connectAll(GroupSocket *);
    void disconnectAll(GroupSocket *);

    using ClientList = std::vector<QPointer<GroupSocket>>;
    ClientList clientsList{};
    GroupTcpServer server;
    GroupPortMapper portMapper;
};
