// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientTelnet.h"

#include "../configuration/configuration.h"
#include "../global/io.h"
#include "../global/utils.h"
#include "../proxy/TextCodec.h"

#include <limits>
#include <tuple>

#include <QApplication>
#include <QByteArray>
#include <QHostAddress>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

ClientTelnetOutputs::~ClientTelnetOutputs() = default;

ClientTelnet::ClientTelnet(ClientTelnetOutputs &output)
    : AbstractTelnet(TextCodecStrategyEnum::FORCE_UTF_8, TelnetTermTypeBytes{"MMapper"})
    , m_output{output}
{
    auto &socket = m_socket;
    QObject::connect(&socket, &QAbstractSocket::connected, &m_dummy, [this]() { onConnected(); });
    QObject::connect(&socket, &QAbstractSocket::disconnected, &m_dummy, [this]() {
        onDisconnected();
    });

    QObject::connect(&socket, &QIODevice::readyRead, &m_dummy, [this]() { onReadyRead(); });
    QObject::connect(&socket,
                     QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
                     &m_dummy,
                     [this](QAbstractSocket::SocketError err) { onError(err); });
}

ClientTelnet::~ClientTelnet()
{
    m_socket.disconnectFromHost();
}

void ClientTelnet::connectToHost()
{
    auto &socket = m_socket;
    if (socket.state() == QAbstractSocket::ConnectedState) {
        return;
    }

    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
    }

    socket.connectToHost(QHostAddress::LocalHost, getConfig().connection.localPort);
    socket.waitForConnected(3000);
}

void ClientTelnet::onConnected()
{
    reset();
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    m_output.connected();
}

void ClientTelnet::disconnectFromHost()
{
    m_socket.disconnectFromHost();
}

void ClientTelnet::onDisconnected()
{
    reset();
    m_output.echoModeChanged(true);
    m_output.disconnected();
}

void ClientTelnet::onError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError) {
        // The connection closing isn't an error
        return;
    }

    QString err = m_socket.errorString();
    m_socket.abort();
    m_output.socketError(err);
}

void ClientTelnet::sendToMud(const QString &data)
{
    submitOverTelnet(data, false);
}

void ClientTelnet::virt_sendRawData(const TelnetIacBytes &data)
{
    m_socket.write(data.getQByteArray());
}

void ClientTelnet::onWindowSizeChanged(const int width, const int height)
{
    auto &current = m_currentNaws;
    if (current.width == width && current.height == height) {
        return;
    }

    // remember the size - we'll need it if NAWS is currently disabled but will
    // be enabled. Also remember it if no connection exists at the moment;
    // we won't be called again when connecting
    current.width = width;
    current.height = height;

    if (getOptions().myOptionState[OPT_NAWS]) {
        // only if we have negotiated this option
        sendWindowSizeChanged(width, height);
    }
}

void ClientTelnet::onReadyRead()
{
    // REVISIT: check return value?
    std::ignore = io::readAllAvailable(m_socket, m_buffer, [this](const QByteArray &byteArray) {
        assert(!byteArray.isEmpty());
        onReadInternal(TelnetIacBytes{byteArray});
    });
}

void ClientTelnet::virt_sendToMapper(const RawBytes &data, bool /*goAhead*/)
{
    // The encoding for the built-in client is always Utf8.
    assert(getEncoding() == CharacterEncodingEnum::UTF8);
    QString out = QString::fromUtf8(data.getQByteArray());

    // Replace BEL character with an application beep
    // REVISIT: This seems like the WRONG place to do this.
    static constexpr const QChar BEL = mmqt::QC_ALERT;
    if (out.contains(BEL)) {
        out.remove(BEL);
        QApplication::beep();
    }

    // REVISIT: Why is virt_sendToMapper() calling sendToUser()? One needs to be renamed?
    m_output.sendToUser(out);
}

void ClientTelnet::virt_receiveEchoMode(const bool mode)
{
    m_output.echoModeChanged(mode);
}
