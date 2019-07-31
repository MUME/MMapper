#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QByteArray>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTcpSocket>
#include <QtCore>
#include <QtGlobal>

#include "../global/io.h"

class UserTelnet;
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

// #define PROXY_STREAM_DEBUG_INPUT_TO_FILE

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
    ~Proxy();

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
#ifdef PROXY_STREAM_DEBUG_INPUT_TO_FILE
    QDataStream *debugStream = nullptr;
    QFile *file = nullptr;
#endif

    io::null_padded_buffer<(1 << 13)> m_buffer;

    qintptr m_socketDescriptor = 0;
    MumeSocket *m_mudSocket = nullptr;
    QScopedPointer<QTcpSocket> m_userSocket;

    bool m_serverConnected = false;

    UserTelnet *m_userTelnet = nullptr;
    MudTelnet *m_mudTelnet = nullptr;
    TelnetFilter *m_telnetFilter = nullptr;
    MpiFilter *m_mpiFilter = nullptr;
    MumeXmlParser *m_parserXml = nullptr;
    RemoteEdit *m_remoteEdit = nullptr;

    MapData *m_mapData = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    MumeClock *m_mumeClock = nullptr;
    MapCanvas *m_mapCanvas = nullptr;

    ConnectionListener *m_listener = nullptr;
};
