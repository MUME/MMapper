/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
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

#include "mumesocket.h"

#include <QByteArray>
#include <QMessageLogContext>
#include <QSslSocket>
#include <QString>
#include <QtNetwork>

#include "../configuration/configuration.h"
#include "../global/io.h"

void MumeSocket::onConnect()
{
    emit connected();
}

void MumeSocket::onDisconnect()
{
    emit disconnected();
}

void MumeSocket::onError(QAbstractSocket::SocketError e)
{
    if (e != QAbstractSocket::RemoteHostClosedError) {
        emit socketError(e);
    }
}

MumeSslSocket::MumeSslSocket(QObject *parent)
    : MumeSocket(parent)
    , m_socket(new QSslSocket(this))
    , m_timer(new QTimer(this))
{
    m_socket->setProtocol(QSsl::TlsV1_2OrLater);
    m_socket->setPeerVerifyMode(QSslSocket::QueryPeer);
    connect(m_socket,
            static_cast<void (QAbstractSocket::*)()>(&QAbstractSocket::connected),
            this,
            &MumeSslSocket::onConnect);
    connect(m_socket, &QIODevice::readyRead, this, &MumeSslSocket::onReadyRead);
    connect(m_socket, &QAbstractSocket::disconnected, this, &MumeSslSocket::onDisconnect);
    connect(m_socket, &QSslSocket::encrypted, this, &MumeSslSocket::onEncrypted);
    connect(m_socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::error),
            this,
            &MumeSslSocket::onError);
    connect(m_socket, &QSslSocket::peerVerifyError, this, &MumeSslSocket::onPeerVerifyError);

    m_timer->setInterval(5000);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &MumeSslSocket::checkTimeout);
}

MumeSslSocket::~MumeSslSocket()
{
    if (m_socket != nullptr) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
    m_socket = nullptr;
    if (m_timer != nullptr) {
        m_timer->stop();
        delete m_timer;
    }
    m_timer = nullptr;
}

void MumeSslSocket::connectToHost()
{
    const auto &settings = Config().connection;
    m_socket->connectToHostEncrypted(settings.remoteServerName,
                                     settings.remotePort,
                                     QIODevice::ReadWrite);
    m_timer->start();
}

void MumeSslSocket::disconnectFromHost()
{
    m_timer->stop();
    m_socket->disconnectFromHost();
}

void MumeSslSocket::onConnect()
{
    emit log("Proxy", "Negotiating handshake with server ...");
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);

    // Restart timer to check if encryption has successfully finished
    m_timer->start();
}

void MumeSslSocket::onError(QAbstractSocket::SocketError e)
{
    qWarning() << "MumeSslSocket error:" << m_socket->errorString();
    MumeSocket::onError(e);
}

void MumeSslSocket::onEncrypted()
{
    m_timer->stop();
    emit log("Proxy", "Connection now encrypted ...");
    MumeSocket::onConnect();
}

void MumeSslSocket::onPeerVerifyError(const QSslError &error)
{
    emit log("Proxy", "<b>WARNING:</b> " + error.errorString());
    // REVISIT: Why is this "Unknown error" sometimes?
    qWarning() << "onPeerVerifyError" << m_socket->errorString();

    if (m_socket->peerVerifyMode() >= QSslSocket::VerifyPeer) {
        m_socket->close();
        emit socketError(QAbstractSocket::SslHandshakeFailedError);
    }
}

void MumeSslSocket::onReadyRead()
{
    if (m_socket != nullptr)
        io::readAllAvailable(*m_socket, m_buffer, [this](QByteArray byteArray) {
            if (byteArray.size() != 0)
                emit processMudStream(byteArray);
        });
}

void MumeSslSocket::checkTimeout()
{
    switch (m_socket->state()) {
    case QAbstractSocket::HostLookupState:
        onError(QAbstractSocket::HostNotFoundError);
        return;

    case QAbstractSocket::ConnectingState:
        onError(QAbstractSocket::SocketTimeoutError);
        return;

    case QAbstractSocket::ConnectedState:
        if (!m_socket->isEncrypted()) {
            emit socketError(QAbstractSocket::SslHandshakeFailedError);
            return;
        }
    default:
        break;
    }
}

void MumeSslSocket::sendToMud(const QByteArray &ba)
{
    m_socket->write(ba.data(), ba.size());
    m_socket->flush();
}

void MumeTcpSocket::connectToHost()
{
    const auto &settings = Config().connection;
    m_socket->connectToHost(settings.remoteServerName, settings.remotePort, QIODevice::ReadWrite);
    m_timer->start();
}

void MumeTcpSocket::onConnect()
{
    m_timer->stop();
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);
    MumeSocket::onConnect();
}
