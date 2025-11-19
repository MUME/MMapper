// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "VirtualSocket.h"

#include <algorithm>
#include <stdexcept>

#include <QPointer>
#include <QTimer>

VirtualSocket::VirtualSocket(QObject *parent)
    : AbstractSocket(parent)
{
    open(QIODevice::ReadWrite);
}

VirtualSocket::~VirtualSocket()
{
    disconnectFromHost();
}

void VirtualSocket::connectToPeer(VirtualSocket *peer)
{
    assert(peer != nullptr && this != peer);

    if (m_peer != nullptr || peer->m_peer != nullptr) {
        throw std::runtime_error("already connected");
    }

    m_peer = peer;
    peer->m_peer = this;
    QObject::connect(peer, &QObject::destroyed, this, &VirtualSocket::slot_onPeerDestroyed);
    emit sig_connected();
    emit peer->sig_connected();
}

void VirtualSocket::virt_flush() {}

void VirtualSocket::virt_disconnectFromHost()
{
    if (!m_peer) {
        return;
    }

    QPointer<VirtualSocket> peerToNotify = m_peer;
    if (m_peer) {
        m_peer->m_peer = nullptr;
    }
    m_peer = nullptr;

    emit sig_disconnected();
    if (peerToNotify) {
        emit peerToNotify->sig_disconnected();
    }
}

bool VirtualSocket::virt_isConnected() const
{
    return m_peer != nullptr && m_peer->m_peer != nullptr;
}

void VirtualSocket::slot_onPeerDestroyed()
{
    if (m_peer) {
        m_peer = nullptr;
        emit sig_disconnected();
    }
}

void VirtualSocket::writeToPeer(const QByteArray &data)
{
    m_buffer.append(data);
    QTimer::singleShot(0, this, [this]() { emit readyRead(); });
}

qint64 VirtualSocket::bytesAvailable() const
{
    return m_buffer.size();
}

qint64 VirtualSocket::readData(char *data, qint64 maxlen)
{
    auto len = std::min(maxlen, static_cast<qint64>(m_buffer.size()));
    memcpy(data, m_buffer.constData(), static_cast<size_t>(len));
    m_buffer.remove(0, len);
    return len;
}

qint64 VirtualSocket::writeData(const char *data, qint64 len)
{
    if (m_peer) {
        m_peer->writeToPeer(QByteArray(data, len));
    }
    return len;
}
