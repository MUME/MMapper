#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <QIODevice>

class NODISCARD_QOBJECT AbstractSocket : public QIODevice
{
    Q_OBJECT

public:
    explicit AbstractSocket(QObject *parent = nullptr)
        : QIODevice(parent)
    {}
    ~AbstractSocket() override = default;

public:
    void flush() { virt_flush(); }
    void disconnectFromHost() { virt_disconnectFromHost(); }
    NODISCARD bool isConnected() const { return virt_isConnected(); }

private:
    virtual void virt_flush() = 0;
    virtual void virt_disconnectFromHost() = 0;
    NODISCARD virtual bool virt_isConnected() const = 0;

signals:
    void sig_connected();
    void sig_disconnected();
};
