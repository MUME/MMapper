#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QByteArray>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTcpSocket>
#include <QtCore>
#include <QtGlobal>

#include "../global/WeakHandle.h"
#include "../global/io.h"
#include "../pandoragroup/GroupManagerApi.h"
#include "ProxyParserApi.h"

class ConnectionListener;
class MapCanvas;
class MapData;
class Mmapper2Group;
class Mmapper2PathMachine;
class MpiFilter;
class MudTelnet;
class MumeClock;
class MumeSocket;
class MumeXmlParser;
class PrespammedPath;
class QDataStream;
class QFile;
class QTcpSocket;
class RemoteEdit;
class TelnetFilter;
class UserTelnet;

#undef ERROR // Bad dog, Microsoft; bad dog!!!

class Proxy final : public QObject
{
    Q_OBJECT
public:
    explicit Proxy(MapData *,
                   Mmapper2PathMachine *,
                   PrespammedPath *,
                   Mmapper2Group *,
                   MumeClock *,
                   MapCanvas *,
                   qintptr &,
                   ConnectionListener *);
    ~Proxy() override;

public slots:
    void start();

    void processUserStream();
    void userTerminatedConnection();
    void mudTerminatedConnection();

    void sendToMud(const QByteArray &);
    void sendToUser(const QByteArray &);

    void onMudError(const QString &);
    void onMudConnected();

signals:
    void log(const QString &, const QString &);

    void analyzeUserStream(const QByteArray &);
    void analyzeMudStream(const QByteArray &);

private:
    friend ProxyParserApi;
    bool isConnected() const;
    void connectToMud();
    void disconnectFromMud();

private:
    io::buffer<(1 << 13)> m_buffer;
    WeakHandleLifetime<Proxy> m_weakHandleLifetime{*this};
    ProxyParserApi m_proxyParserApi{m_weakHandleLifetime.getWeakHandle()};

    MapData *const m_mapData;
    Mmapper2PathMachine *const m_pathMachine;
    PrespammedPath *const m_prespammedPath;
    Mmapper2Group *const m_groupManager;
    MumeClock *const m_mumeClock;
    MapCanvas *const m_mapCanvas;
    ConnectionListener *const m_listener;
    const qintptr m_socketDescriptor;

    // initialized in ctor
    QPointer<RemoteEdit> m_remoteEdit;

    // initialized in start()
    QPointer<QTcpSocket> m_userSocket;
    QPointer<UserTelnet> m_userTelnet;
    QPointer<MudTelnet> m_mudTelnet;
    QPointer<TelnetFilter> m_telnetFilter;
    QPointer<MpiFilter> m_mpiFilter;
    QPointer<MumeXmlParser> m_parserXml;
    QPointer<MumeSocket> m_mudSocket;

    enum class ServerStateEnum {
        INITIALIZED,
        OFFLINE,
        CONNECTING,
        CONNECTED,
        DISCONNECTING,
        DISCONNECTED,
        ERROR,
    };

    ServerStateEnum m_serverState = ServerStateEnum::INITIALIZED;
};
