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

#include <QApplication>
#include <QByteArray>
#include <QHostAddress>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

ClientTelnet::ClientTelnet(QObject *const parent)
    : AbstractTelnet(TextCodecStrategyEnum::FORCE_LATIN_1, parent, "MMapper")
{
    connect(&socket, &QAbstractSocket::connected, this, &ClientTelnet::slot_onConnected);
    connect(&socket, &QAbstractSocket::disconnected, this, &ClientTelnet::slot_onDisconnected);
    connect(&socket, &QIODevice::readyRead, this, &ClientTelnet::slot_onReadyRead);
    connect(&socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this,
            &ClientTelnet::slot_onError);
}

ClientTelnet::~ClientTelnet()
{
    socket.disconnectFromHost();
}

void ClientTelnet::connectToHost()
{
    if (socket.state() == QAbstractSocket::ConnectedState)
        return;

    if (socket.state() != QAbstractSocket::UnconnectedState)
        socket.abort();

    socket.connectToHost(QHostAddress::LocalHost, getConfig().connection.localPort);
    socket.waitForConnected(3000);
}

void ClientTelnet::slot_onConnected()
{
    reset();
    socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    emit sig_connected();
}

void ClientTelnet::disconnectFromHost()
{
    socket.disconnectFromHost();
}

void ClientTelnet::slot_onDisconnected()
{
    reset();
    emit sig_echoModeChanged(true);
    emit sig_disconnected();
}

void ClientTelnet::slot_onError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError) {
        // The connection closing isn't an error
        return;
    }
    QString err = socket.errorString();
    socket.abort();
    emit sig_socketError(err);
}

void ClientTelnet::slot_sendToMud(const QString &data)
{
    submitOverTelnet(mmqt::toStdStringLatin1(data), false);
}

void ClientTelnet::virt_sendRawData(const std::string_view data)
{
    sentBytes += data.length();
    socket.write(mmqt::toQByteArrayLatin1(data));
}

void ClientTelnet::slot_onWindowSizeChanged(int x, int y)
{
    if (current.x == x && current.y == y)
        return;

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

void ClientTelnet::slot_onReadyRead()
{
    // REVISIT: check return value?
    MAYBE_UNUSED const auto ignored = //
        io::readAllAvailable(socket, buffer, [this](const QByteArray &byteArray) {
            onReadInternal(byteArray);
        });
}

void ClientTelnet::virt_sendToMapper(const QByteArray &data, bool /*goAhead*/)
{
    QString out = [&data] {
        switch (getConfig().general.characterEncoding) {
        case CharacterEncodingEnum::UTF8:
            return QString::fromUtf8(data);
        case CharacterEncodingEnum::LATIN1:
        case CharacterEncodingEnum::ASCII:
        default:
            return QString::fromLatin1(data);
        }
    }();

    // Replace BEL character with an application beep
    static constexpr const QChar BEL = mmqt::QC_ALERT;
    if (out.contains(BEL)) {
        out.remove(BEL);
        QApplication::beep();
    }

    emit sig_sendToUser(out);
}

void ClientTelnet::virt_receiveEchoMode(const bool mode)
{
    emit sig_echoModeChanged(mode);
}
