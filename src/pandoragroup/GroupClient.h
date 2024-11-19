#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupCommunicator.h"
#include "GroupSocket.h"

#include <QVariantMap>

class GroupClient final : public CGroupCommunicator
{
    Q_OBJECT
    friend class GroupSocket;

private:
    ProtocolVersion m_proposedProtocolVersion = PROTOCOL_VERSION_102;
    bool m_clientConnected = false;
    int m_reconnectAttempts = 3;
    GroupSocket m_socket;

public:
    explicit GroupClient(Mmapper2Group *parent);
    ~GroupClient() final;

public slots:
    void slot_errorInConnection(GroupSocket *socket, const QString &);
    void slot_connectionEncrypted();
    void slot_connectionEstablished();

private:
    NODISCARD bool virt_start() final;
    void virt_stop() final;

private:
    void virt_connectionClosed(GroupSocket &socket) final;
    void virt_kickCharacter(const QByteArray &) final;
    void virt_retrieveData(GroupSocket &socket, MessagesEnum message, const QVariantMap &data) final;
    void virt_sendCharRename(const QVariantMap &map) final;
    void virt_sendCharUpdate(const QVariantMap &map) final;
    void virt_sendGroupTellMessage(const QVariantMap &map) final;

private:
    void sendHandshake(const QVariantMap &data);
    void sendLoginInformation();
    void tryReconnecting();
    void receiveGroupInformation(const QVariantMap &data);
};
