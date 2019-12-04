// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientTelnet.h"

#include <limits>
#include <QApplication>
#include <QByteArray>
#include <QHostAddress>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

#include "../configuration/configuration.h"
#include "../global/io.h"
#include "../global/utils.h"
#include "../proxy/TextCodec.h"

static TextCodecStrategyEnum getStrategyFromConfig()
{
    const auto characterEncoding = getConfig().general.characterEncoding;
    switch (characterEncoding) {
    case CharacterEncodingEnum::ASCII:
        return TextCodecStrategyEnum::FORCE_US_ASCII;
    case CharacterEncodingEnum::LATIN1:
        return TextCodecStrategyEnum::FORCE_LATIN_1;
    case CharacterEncodingEnum::UTF8:
        return TextCodecStrategyEnum::FORCE_UTF_8;
    }
    return TextCodecStrategyEnum::AUTO_SELECT_CODEC;
}

ClientTelnet::ClientTelnet(QObject *const parent)
    : AbstractTelnet(TextCodec(getStrategyFromConfig()), false, parent, "MMapper")
{
    connect(&socket, &QAbstractSocket::connected, this, &ClientTelnet::onConnected);
    connect(&socket, &QAbstractSocket::disconnected, this, &ClientTelnet::onDisconnected);
    connect(&socket, &QIODevice::readyRead, this, &ClientTelnet::onReadyRead);
    connect(&socket,
            SIGNAL(error(QAbstractSocket::SocketError)),
            this,
            SLOT(onError(QAbstractSocket::SocketError)));
}

ClientTelnet::~ClientTelnet()
{
    socket.disconnectFromHost();
}

void ClientTelnet::connectToHost()
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
    }
    socket.connectToHost(QHostAddress::LocalHost, getConfig().connection.localPort);
    socket.waitForConnected(3000);
}

void ClientTelnet::onConnected()
{
    reset();
    socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    emit connected();
}

void ClientTelnet::disconnectFromHost()
{
    socket.disconnectFromHost();
}

void ClientTelnet::onDisconnected()
{
    reset();
    emit echoModeChanged(true);
    emit disconnected();
}

void ClientTelnet::onError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError) {
        // The connection closing isn't an error
        return;
    }
    QString err = socket.errorString();
    socket.abort();
    emit socketError(err);
}

void ClientTelnet::sendToMud(const QString &data)
{
    // MMapper will later convert to Latin-1 within UserTelnet
    const QByteArray ba = textCodec.fromUnicode(data);
    submitOverTelnet(ba, false);
}

void ClientTelnet::sendRawData(const QByteArray &data)
{
    sentBytes += data.length();
    socket.write(data);
}

void ClientTelnet::onWindowSizeChanged(int x, int y)
{
    // remember the size - we'll need it if NAWS is currently disabled but will
    // be enabled. Also remember it if no connection exists at the moment;
    // we won't be called again when connecting
    current.x = x;
    current.y = y;

    if (myOptionState[OPT_NAWS]) {
        // only if we have negotiated this option
        sendWindowSizeChanged(x, y);
    }
}

void ClientTelnet::onReadyRead()
{
    io::readAllAvailable(socket, buffer, [this](const QByteArray &byteArray) {
        onReadInternal(byteArray);
    });
}

void ClientTelnet::sendToMapper(const QByteArray &data, bool /*goAhead*/)
{
    QString out = textCodec.toUnicode(data);

    // Replace BEL character with an application beep
    static constexpr const QChar BEL('\a');
    if (out.contains(BEL)) {
        out.remove(BEL);
        QApplication::beep();
    }

    emit sendToUser(out);
}

void ClientTelnet::receiveEchoMode(const bool mode)
{
    emit echoModeChanged(mode);
}
