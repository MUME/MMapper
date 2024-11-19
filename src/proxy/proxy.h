#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/AnsiOstream.h"
#include "../global/Signal2.h"
#include "../global/WeakHandle.h"
#include "../global/io.h"
#include "../observer/gameobserver.h"
#include "../pandoragroup/GroupManagerApi.h"
#include "../timers/CTimers.h"
#include "GmcpMessage.h"
#include "ProxyParserApi.h"
#include "TaggedBytes.h"

#include <memory>
#include <sstream>

#include <QByteArray>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTcpSocket>
#include <QtCore>
#include <QtGlobal>

class CTimers;
class ConnectionListener;
class MapCanvas;
class MapData;
class Mmapper2Group;
class Mmapper2PathMachine;
class MpiFilter;
class MudTelnet;
class MudTelnetFilter;
class MumeClock;
class MumeSocket;
class MumeXmlParser;
class PrespammedPath;
class QTcpSocket;
class RemoteEdit;
class RoomManager;
class UserTelnet;
class UserTelnetFilter;

class NODISCARD_QOBJECT Proxy final : public QObject
{
    Q_OBJECT

private:
    io::buffer<(1 << 13)> m_buffer;
    WeakHandleLifetime<Proxy> m_weakHandleLifetime{*this};
    ProxyParserApi m_proxyParserApi{m_weakHandleLifetime.getWeakHandle()};

    MapData &m_mapData;
    Mmapper2PathMachine &m_pathMachine;
    PrespammedPath &m_prespammedPath;
    Mmapper2Group &m_groupManager;
    MumeClock &m_mumeClock;
    MapCanvas &m_mapCanvas;
    GameObserver &m_gameObserver;
    const qintptr m_socketDescriptor;
    ConnectionListener &m_listener;

    // REVISIT: This might be in the wrong spot; it's not part of the pipeline
    // because it's intended for sendXXX within the lifetime of this object.
    Signal2Lifetime m_lifetime;

    // initialized in ctor
    QPointer<RemoteEdit> m_remoteEdit;

    // initialized in start()
    QPointer<QTcpSocket> m_userSocket;
    QPointer<UserTelnet> m_userTelnet;
    QPointer<MudTelnet> m_mudTelnet;
    QPointer<UserTelnetFilter> m_userTelnetFilter;
    QPointer<MudTelnetFilter> m_mudTelnetFilter;
    QPointer<MpiFilter> m_mpiFilter;
    QPointer<MumeXmlParser> m_parserXml;
    QPointer<MumeSocket> m_mudSocket;
    QPointer<CTimers> m_timers;

    enum class NODISCARD ServerStateEnum {
        Initialized,
        Offline,
        Connecting,
        Connected,
        Disconnecting,
        Disconnected,
        Error,
    };

    ServerStateEnum m_serverState = ServerStateEnum::Initialized;

public:
    explicit Proxy(MapData &,
                   Mmapper2PathMachine &,
                   PrespammedPath &,
                   Mmapper2Group &,
                   MumeClock &,
                   MapCanvas &,
                   GameObserver &,
                   qintptr &,
                   ConnectionListener &);
    ~Proxy() final;

private:
    friend ProxyParserApi;
    NODISCARD bool isConnected() const;
    void connectToMud();
    void disconnectFromMud();
    void sendToUser(const QString &ba) { emit sig_sendToUser(ba, false); }
    void sendToMud(const QString &ba) { emit sig_sendToMud(ba); }
    void gmcpToUser(const GmcpMessage &msg) { emit sig_gmcpToUser(msg); }
    void gmcpToMud(const GmcpMessage &msg) { emit sig_gmcpToMud(msg); }
    NODISCARD bool isGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const;
    void log(const QString &msg) { emit sig_log("Proxy", msg); }

    class NODISCARD AnsiHelper final
    {
    public:
        using Callback = Signal2<std::string>;

    private:
        Callback m_callback;
        std::ostringstream m_oss;
        AnsiOstream m_aos{m_oss};

    public:
        explicit AnsiHelper(const Signal2Lifetime &lifetime, Callback::Function fn)
        {
            m_callback.connect(lifetime, std::move(fn));
        }
        ~AnsiHelper()
        {
            auto str = std::move(m_oss).str();
            m_callback.invoke(std::move(str));
        }
        DELETE_CTORS_AND_ASSIGN_OPS(AnsiHelper);

        template<typename T>
        void write(T &&x)
        {
            m_aos.write(std::forward<T>(x));
        }
        template<typename T>
        void writeWithColor(const RawAnsi &ansi, T &&x)
        {
            m_aos.writeWithColor(ansi, std::forward<T>(x));
        }
    };

    NODISCARD AnsiHelper getSendToUserAnsiOstream();
    void sendNewlineToUser() { sendToUser("\n"); }
    void sendWelcomeToUser();
    void sendErrorToUser(std::string_view msg);
    void sendStatusToUser(std::string_view msg);
    void sendSyntaxHintToUser(std::string_view before, std::string_view cmd, std::string_view after);

signals:
    void sig_log(const QString &, const QString &);

    void sig_analyzeUserStream(const TelnetIacBytes &);
    void sig_analyzeMudStream(const TelnetIacBytes &);

    void sig_sendToMud(const QString &);
    void sig_sendToUser(const QString &, bool);

    void sig_gmcpToMud(const GmcpMessage &);
    void sig_gmcpToUser(const GmcpMessage &);

public slots:
    void slot_start();

    void slot_processUserStream();
    void slot_userTerminatedConnection();
    void slot_mudTerminatedConnection();

    void slot_onSendToMudSocket(const TelnetIacBytes &);
    void slot_onSendToUserSocket(const TelnetIacBytes &);

    void slot_onMudError(const QString &);
    void slot_onMudConnected();

    void slot_onSendGameTimeToClock(int year, const std::string &month, int day, int hour);
};
