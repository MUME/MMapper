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

public:
    explicit GroupClient(Mmapper2Group *parent);
    ~GroupClient() override;

public slots:
    void errorInConnection(GroupSocket *socket, const QString &);
    void retrieveData(GroupSocket *socket, MessagesEnum message, const QVariantMap &data) override;
    void connectionClosed(GroupSocket *socket) override;
    void connectionEncrypted();
    void connectionEstablished();

protected:
    void sendGroupTellMessage(const QVariantMap &map) override;
    NODISCARD bool start() override;
    void stop() override;
    void sendCharUpdate(const QVariantMap &map) override;
    void sendCharRename(const QVariantMap &map) override;
    void kickCharacter(const QByteArray &) override;

private:
    void sendHandshake(const QVariantMap &data);
    void sendLoginInformation();
    void tryReconnecting();
    void receiveGroupInformation(const QVariantMap &data);

    ProtocolVersion proposedProtocolVersion = PROTOCOL_VERSION_102;
    bool clientConnected = false;
    int reconnectAttempts = 3;
    GroupSocket socket;
};
