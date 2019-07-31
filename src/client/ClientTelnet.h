#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    QTcpSocket socket;
};
