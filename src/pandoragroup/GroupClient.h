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

#ifndef GROUPCLIENT_H
#define GROUPCLIENT_H

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
    void retrieveData(GroupSocket *socket, const Messages message, const QVariantMap &data) override;
    void connectionClosed(GroupSocket *socket) override;
    void connectionEncrypted(GroupSocket *socket);
    void connectionEstablished(GroupSocket *socket);

protected:
    void sendGroupTellMessage(const QVariantMap &map) override;
    bool start() override;
    void stop() override;
    void sendCharUpdate(const QVariantMap &map) override;
    void sendCharRename(const QVariantMap &map) override;
    bool kickCharacter(const QByteArray &) override;

private:
    void sendHandshake(GroupSocket *socket, const QVariantMap &data);
    void sendLoginInformation(GroupSocket *socket);

    ProtocolVersion proposedProtocolVersion = PROTOCOL_VERSION_102;
    bool clientConnected = false;
    GroupSocket client;
};

#endif // GROUPCLIENT_H
