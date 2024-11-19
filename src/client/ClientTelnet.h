#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/io.h"
#include "../proxy/AbstractTelnet.h"

#include <QAbstractSocket>
#include <QObject>
#include <QTcpSocket>

class ClientTelnet final : public AbstractTelnet
{
    Q_OBJECT

private:
    io::buffer<(1 << 15)> buffer;
    QTcpSocket socket;

public:
    explicit ClientTelnet(QObject *parent);
    ~ClientTelnet() final;

    void connectToHost();
    void disconnectFromHost();

public slots:
    /** Window size has changed - informs the server about it */
    void slot_onWindowSizeChanged(int x, int y);

    /** Prepares data, doubles IACs, sends it using sendRawData. */
    void slot_sendToMud(const QString &data);

protected slots:
    void slot_onConnected();
    void slot_onDisconnected();
    void slot_onError(QAbstractSocket::SocketError);

    /** Reads, parses telnet, and so forth */
    void slot_onReadyRead();

signals:
    /** Submits Telnet/text data back to the client */
    void sig_sendToUser(const QString &data);

    /** toggles echo mode for passwords */
    void sig_echoModeChanged(bool);
    void sig_disconnected();
    void sig_connected();
    void sig_socketError(const QString &);

private:
    void virt_sendToMapper(const QByteArray &, bool goAhead) final;
    void virt_receiveEchoMode(bool) final;
    void virt_sendRawData(std::string_view data) final;
};
