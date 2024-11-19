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

class NODISCARD_QOBJECT ClientTelnet final : public AbstractTelnet
{
    Q_OBJECT

private:
    io::buffer<(1 << 15)> m_buffer;
    QTcpSocket m_socket;

public:
    explicit ClientTelnet(QObject *parent);
    ~ClientTelnet() final;

    void connectToHost();
    void disconnectFromHost();

private:
    void virt_sendToMapper(const RawBytes &, bool goAhead) final;
    void virt_receiveEchoMode(bool) final;
    void virt_sendRawData(const TelnetIacBytes &data) final;

signals:
    /** Submits Telnet/text data back to the client */
    void sig_sendToUser(const QString &data);

    /** toggles echo mode for passwords */
    void sig_echoModeChanged(bool);
    void sig_disconnected();
    void sig_connected();
    void sig_socketError(const QString &);

public slots:
    /** Window size has changed - informs the server about it */
    void slot_onWindowSizeChanged(int width, int height);

    /** Prepares data, doubles IACs, sends it using sendRawData. */
    void slot_sendToMud(const QString &data);

protected slots:
    void slot_onConnected();
    void slot_onDisconnected();
    void slot_onError(QAbstractSocket::SocketError);

    /** Reads, parses telnet, and so forth */
    void slot_onReadyRead();
};
