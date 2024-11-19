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

class NODISCARD_QOBJECT GroupTcpServer final : public QTcpServer
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

class NODISCARD_QOBJECT GroupServer final : public CGroupCommunicator
{
    Q_OBJECT

private:
    using ClientList = std::vector<QPointer<GroupSocket>>;
    ClientList m_clientsList{};
    GroupTcpServer m_server;
    GroupPortMapper m_portMapper;

public:
    explicit GroupServer(Mmapper2Group *parent);
    ~GroupServer() final;

private:
    NODISCARD ClientList &filterClientList();
    void relayMessage(GroupSocket &socket, const MessagesEnum message, const QVariantMap &data)
    {
        slot_relayMessage(&socket, message, data);
    }

protected:
    void sendRemoveUserNotification(GroupSocket &socket, const QString &name);

private:
    NODISCARD bool virt_start() final;
    void virt_stop() final;

private:
    void virt_connectionClosed(GroupSocket &socket) final;
    void virt_kickCharacter(const QString &) final;
    void virt_retrieveData(GroupSocket &socket, MessagesEnum message, const QVariantMap &data) final;
    void virt_sendCharRename(const QVariantMap &map) final;
    void virt_sendCharUpdate(const QVariantMap &map) final;
    void virt_sendGroupTellMessage(const QVariantMap &root) final;

private:
    void parseHandshake(GroupSocket &socket, const QVariantMap &data);
    void parseLoginInformation(GroupSocket &socket, const QVariantMap &data);
    void sendGroupInformation(GroupSocket &socket);
    void kickConnection(GroupSocket &socket, const QString &message);

private:
    void sendToAll(const QString &);
    void sendToAllExceptOne(GroupSocket *exception, const QString &);
    void closeAll();
    void closeOne(GroupSocket &target);
    void connectAll(GroupSocket &);
    void disconnectAll(GroupSocket &);

protected slots:
    void slot_relayMessage(GroupSocket *socket, MessagesEnum message, const QVariantMap &data);
    void slot_connectionEstablished(GroupSocket *socket);
    void slot_onRevokeWhitelist(const GroupSecret &secret);
    void slot_onIncomingConnection(qintptr socketDescriptor);
    void slot_errorInConnection(GroupSocket *, const QString &);
};
