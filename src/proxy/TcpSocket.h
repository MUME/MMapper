#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractSocket.h"

#include <QTcpSocket>

class NODISCARD_QOBJECT TcpSocket final : public AbstractSocket
{
    Q_OBJECT

public:
    explicit TcpSocket(qintptr socketDescriptor, QObject *parent = nullptr);
    ~TcpSocket() final;

    void virt_flush() override;
    void virt_disconnectFromHost() override;
    NODISCARD bool virt_isConnected() const override;

    NODISCARD qint64 bytesAvailable() const override;

protected:
    NODISCARD qint64 readData(char *data, qint64 maxlen) override;
    NODISCARD qint64 writeData(const char *data, qint64 len) override;

private:
    QTcpSocket m_socket;
};
