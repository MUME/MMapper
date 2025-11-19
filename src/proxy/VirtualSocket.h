#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractSocket.h"

#include <QByteArray>
#include <QPointer>

class NODISCARD_QOBJECT VirtualSocket final : public AbstractSocket
{
    Q_OBJECT

public:
    explicit VirtualSocket(QObject *parent = nullptr);
    ~VirtualSocket() final;

    void virt_flush() override;
    void virt_disconnectFromHost() override;
    NODISCARD bool virt_isConnected() const override;

    void connectToPeer(VirtualSocket *peer);

    NODISCARD qint64 bytesAvailable() const override;

protected:
    NODISCARD qint64 readData(char *data, qint64 maxlen) override;
    NODISCARD qint64 writeData(const char *data, qint64 len) override;

private slots:
    void slot_onPeerDestroyed();

private:
    void writeToPeer(const QByteArray &data);
    QPointer<VirtualSocket> m_peer;
    QByteArray m_buffer;
};
