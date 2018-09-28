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

static TextCodecStrategy getStrategyFromConfig()
{
    const auto characterEncoding = getConfig().general.characterEncoding;
    switch (characterEncoding) {
    case CharacterEncoding::ASCII:
        return TextCodecStrategy::FORCE_US_ASCII;
    case CharacterEncoding::LATIN1:
        return TextCodecStrategy::FORCE_LATIN_1;
    case CharacterEncoding::UTF8:
        return TextCodecStrategy::FORCE_UTF_8;
    }
    return TextCodecStrategy::AUTO_SELECT_CODEC;
}

ClientTelnet::ClientTelnet(QObject *const parent)
    : AbstractTelnet(TextCodec(getStrategyFromConfig()), false, parent)
{
    /** MMapper Telnet */
    termType = "MMapper";

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
    socket.deleteLater();
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
    //update counter
    sentBytes += data.length();
    socket.write(data);
}

void ClientTelnet::onWindowSizeChanged(int x, int y)
{
    //remember the size - we'll need it if NAWS is currently disabled but will
    //be enabled. Also remember it if no connection exists at the moment;
    //we won't be called again when connecting
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
