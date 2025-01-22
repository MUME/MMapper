#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Signal2.h"
#include "../global/io.h"
#include "../global/utils.h"
#include "../proxy/AbstractTelnet.h"

#include <QAbstractSocket>
#include <QObject>
#include <QTcpSocket>

struct ClientTelnetOutputs
{
    explicit ClientTelnetOutputs() = default;
    virtual ~ClientTelnetOutputs();
    DELETE_CTORS_AND_ASSIGN_OPS(ClientTelnetOutputs);

public:
    void connected() { virt_connected(); }
    void disconnected() { virt_disconnected(); }
    void socketError(const QString &msg) { virt_socketError(msg); }
    /** toggles echo mode for passwords */
    void echoModeChanged(const bool echo) { virt_echoModeChanged(echo); }
    /** Submits Telnet/text data back to the client */
    void sendToUser(const QString &data) { virt_sendToUser(data); }

private:
    virtual void virt_connected() = 0;
    virtual void virt_disconnected() = 0;
    virtual void virt_socketError(const QString &msg) = 0;
    /** toggles echo mode for passwords */
    virtual void virt_echoModeChanged(bool echo) = 0;
    /** Submits Telnet/text data back to the client */
    virtual void virt_sendToUser(const QString &data) = 0;
};

class NODISCARD ClientTelnet final : AbstractTelnet
{
private:
    ClientTelnetOutputs &m_output;
    io::buffer<(1 << 15)> m_buffer;
    QTcpSocket m_socket;
    QObject m_dummy;

public:
    explicit ClientTelnet(ClientTelnetOutputs &output);
    ~ClientTelnet() final;

private:
    NODISCARD ClientTelnetOutputs &getOutput() { return m_output; }

public:
    void connectToHost();
    void disconnectFromHost();

private:
    void virt_sendToMapper(const RawBytes &, bool goAhead) final;
    void virt_receiveEchoMode(bool) final;
    void virt_sendRawData(const TelnetIacBytes &data) final;

public:
    /** Window size has changed - informs the server about it */
    void onWindowSizeChanged(int width, int height);

    /** Prepares data, doubles IACs, sends it using sendRawData. */
    void sendToMud(const QString &data);

protected:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

    /** Reads, parses telnet, and so forth */
    void onReadyRead();
};
