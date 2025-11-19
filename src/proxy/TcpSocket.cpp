// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "TcpSocket.h"

#include <stdexcept>

TcpSocket::TcpSocket(qintptr socketDescriptor, QObject *parent)
    : AbstractSocket(parent)
{
    if (!m_socket.setSocketDescriptor(socketDescriptor)) {
        throw std::runtime_error("failed to accept user socket");
    }
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, true);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    connect(&m_socket, &QAbstractSocket::connected, this, &AbstractSocket::sig_connected);
    connect(&m_socket, &QAbstractSocket::disconnected, this, &AbstractSocket::sig_disconnected);
    connect(&m_socket, &QIODevice::readyRead, this, &AbstractSocket::readyRead);
    open(QIODevice::ReadWrite);
}

TcpSocket::~TcpSocket()
{
    disconnectFromHost();
}

void TcpSocket::virt_flush()
{
    m_socket.flush();
}

void TcpSocket::virt_disconnectFromHost()
{
    m_socket.disconnectFromHost();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.waitForDisconnected();
    }
}

bool TcpSocket::virt_isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

qint64 TcpSocket::bytesAvailable() const
{
    return m_socket.bytesAvailable();
}

qint64 TcpSocket::readData(char *data, qint64 maxlen)
{
    return m_socket.read(data, maxlen);
}

qint64 TcpSocket::writeData(const char *data, qint64 len)
{
    return m_socket.write(data, len);
}
