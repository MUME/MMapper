#pragma once
/************************************************************************
**
** Authors:   Tomas Mecir <kmuddy@kmuddy.com>
**            Nils Schimmelmann <nschimme@gmail.com>
**
** Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
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

#ifndef CLIENTTELNET_H
#define CLIENTTELNET_H

#include <QAbstractSocket>
#include <QTcpSocket>

#include "../global/io.h"
#include "../proxy/AbstractTelnet.h"

class ClientTelnet final : public AbstractTelnet
{
    Q_OBJECT

public:
    explicit ClientTelnet(QObject *parent = nullptr);
    ~ClientTelnet() override;

    void connectToHost();

    void disconnectFromHost();

public slots:
    /** Window size has changed - informs the server about it */
    void onWindowSizeChanged(int x, int y);

    /** Prepares data, doubles IACs, sends it using sendRawData. */
    void sendToMud(const QString &data);

protected slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

    /** Reads, parses telnet, and so forth */
    void onReadyRead();

signals:
    /** Submits Telnet/text data back to the client */
    void sendToUser(const QString &data);

    /** toggles echo mode for passwords */
    void echoModeChanged(bool);

    void disconnected();
    void connected();
    void socketError(const QString &);

private:
    void sendToMapper(const QByteArray &, bool goAhead) override;
    void receiveEchoMode(bool) override;
    void sendRawData(const QByteArray &data) override;

    io::null_padded_buffer<(1 << 15)> buffer{};
    QTcpSocket socket{};
};

#endif /* CLIENTTELNET_H */
