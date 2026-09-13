#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/ConfigEnums.h"
#include "../global/macros.h"

#include <QObject>
#include <QString>

class HotkeyManager;

// The callbacks Proxy/ConnectionListener need from their owning host, so
// they don't depend on the concrete type MainWindow. The host implements
// this alongside its own QObject base -- see mainwindow.h's `class
// MainWindow final : public QMainWindow, public ProxyHost`.
// ConnectionListener's parent must implement this interface (see proxy.cpp's
// getProxyHost()).
class NODISCARD ProxyHost
{
public:
    virtual ~ProxyHost();

public:
    // Funnels Proxy/parser log messages ("mod", "message") into the host's
    // log surface.
    void log(const QString &mod, const QString &msg) { virt_log(mod, msg); }

    // The mud can request a mapper-mode switch via the MPI protocol (see
    // ProxyMudConnectionApi::virt_onSetMode() in proxy.cpp); the host applies
    // it the same way a user clicking the mapper-mode toolbar buttons would.
    void setMode(const MapModeEnum mode) { virt_setMode(mode); }

    // The parser needs this to expand user-defined hotkey aliases.
    NODISCARD HotkeyManager &getHotkeyManager() const { return virt_getHotkeyManager(); }

    // RemoteEdit needs a QObject parent to outlive the Proxy that spawned it
    // (see Proxy::allocRemoteEdit()'s "Caution: RemoteEdit outlives the
    // proxy" comment); this exposes the host's own QObject identity for that
    // purpose. Implementations should just `return *this;`.
    NODISCARD QObject &asQObject() { return virt_asQObject(); }

private:
    virtual void virt_log(const QString &mod, const QString &msg) = 0;
    virtual void virt_setMode(MapModeEnum mode) = 0;
    NODISCARD virtual HotkeyManager &virt_getHotkeyManager() const = 0;
    NODISCARD virtual QObject &virt_asQObject() = 0;
};
