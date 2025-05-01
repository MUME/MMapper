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
#include "../group/GroupManagerApi.h"
#include "../observer/gameobserver.h"
#include "../parser/SendToUserSourceEnum.h"
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

class AbstractParser;
class CTimers;
class ConnectionListener;
class MainWindow;
class MapCanvas;
class MapData;
class Mmapper2Group;
class Mmapper2PathMachine;
class MpiFilter;
class MpiFilterToMud;
class MudTelnet;
class MumeClock;
class MumeFallbackSocket;
class MumeXmlParser;
class PasswordConfig;
class PrespammedPath;
class ProxyMudConnectionApi;
class ProxyUserGmcpApi;
class QTcpSocket;
class RemoteEdit;
class RoomManager;
class TelnetLineFilter;
class UserTelnet;

struct AbstractParserOutputs;
struct AnsiWarningMessage;
struct GameTime;
struct MpiFilterOutputs;
struct MudTelnetOutputs;
struct MumeSocketOutputs;
struct ParserCommonData;
struct UserTelnetOutputs;

class NODISCARD_QOBJECT Proxy final : public QObject
{
    Q_OBJECT

private:
    io::buffer<(1 << 13)> m_buffer;
    WeakHandleLifetime<Proxy> m_weakHandleLifetime{*this};

    MapData &m_mapData;
    Mmapper2PathMachine &m_pathMachine;
    PrespammedPath &m_prespammedPath;
    Mmapper2Group &m_groupManager;
    MumeClock &m_mumeClock;
    MapCanvas &m_mapCanvas;
    GameObserver &m_gameObserver;
    const qintptr m_socketDescriptor;
    MainWindow &m_mainWindow;

private:
    class UserSocket;
    struct UserSocketOutputs;
    struct NODISCARD Pipeline final
    {
    public:
        struct NODISCARD Outputs final
        {
            struct NODISCARD User final
            {
                std::unique_ptr<UserSocketOutputs> userSocketOutputs;
                std::unique_ptr<UserTelnetOutputs> userTelnetOutputs;
            };
            struct NODISCARD Mud final
            {
                std::unique_ptr<MumeSocketOutputs> mudSocketOutputs;
                std::unique_ptr<MudTelnetOutputs> mudTelnetOutputs;
                std::unique_ptr<MpiFilterOutputs> mpiFilterOutputs;
            };
            std::unique_ptr<AbstractParserOutputs> parserXmlOutputs;
            User user;
            Mud mud;
        };
        Outputs outputs;
        struct NODISCARD Apis final
        {
            std::unique_ptr<ProxyMudConnectionApi> proxyMudConnection;
            std::unique_ptr<ProxyUserGmcpApi> proxyGmcp;
            std::optional<Signal2Lifetime> sendToUserLifetime;
        };
        Apis apis;

        struct NODISCARD Common final
        {
            std::unique_ptr<ParserCommonData> parserCommonData;
        };
        Common common;

    public: // from user: Sock -> Telnet -> LineFilter -> Parser
        struct NODISCARD User final
        {
            std::unique_ptr<UserSocket> userSocket;
            std::unique_ptr<UserTelnet> userTelnet;
            std::unique_ptr<TelnetLineFilter> userTelnetFilter;
            std::unique_ptr<AbstractParser> userParser;
        };
        User user;

    public: // from mud: Sock -> Telnet -> LineFilter -> Mpi -> Parser
        struct NODISCARD Mud final
        {
            std::unique_ptr<MumeFallbackSocket> mudSocket;
            std::unique_ptr<MudTelnet> mudTelnet;
            std::unique_ptr<TelnetLineFilter> mudTelnetFilter;
            std::unique_ptr<MpiFilter> mpiFilterFromMud;
            std::unique_ptr<MpiFilterToMud> mpiFilterToMud;
            std::unique_ptr<MumeXmlParser> mudParser;
            std::unique_ptr<PasswordConfig> passwordConfig;
        };
        Mud mud;

    public:
    };

private:
    std::unique_ptr<Pipeline> m_pipeline;
    // REVISIT: This might be in the wrong spot; it's not part of the pipeline
    // because it's intended for sendXXX within the lifetime of this object.
    Signal2Lifetime m_lifetime;

    // Technically we create this, but we don't "own" it;
    // it outlives this object when the connection closes.
    QPointer<RemoteEdit> m_remoteEdit;

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
    NODISCARD static QPointer<Proxy> allocInit(MapData &,
                                               Mmapper2PathMachine &,
                                               PrespammedPath &,
                                               Mmapper2Group &,
                                               MumeClock &,
                                               MapCanvas &,
                                               GameObserver &,
                                               qintptr &,
                                               ConnectionListener &);

public:
    explicit Proxy(Badge<Proxy>,
                   MapData &,
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
    void init();

private:
    void allocPipelineObjects();
    void destroyPipelineObjects();

    void allocUserSocket();
    void allocMudSocket();
    void allocUserTelnet();
    void allocMudTelnet();
    void allocParser();
    void allocMpiFilter();
    void allocRemoteEdit();

private:
    void processUserStream();
    void userTerminatedConnection();

private:
    void onMudConnected();
    void onMudError(const QString &);
    void mudTerminatedConnection();

private:
    friend ProxyMudConnectionApi;
    NODISCARD bool isConnected() const;
    void connectToMud();
    void disconnectFromMud();

private:
    friend ProxyMudGmcpApi;
    friend ProxyUserGmcpApi;
    NODISCARD bool isMudGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const;
    NODISCARD bool isUserGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const;
    void gmcpToMud(const GmcpMessage &msg);
    void gmcpToUser(const GmcpMessage &msg);

private:
    void sendToMud(const QString &s);
    void sendToUser(SendToUserSourceEnum source, const QString &ba);
    void log(const QString &msg);

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
    void sendNewlineToUser();
    void sendPromptToUser();

    void sendWelcomeToUser();
    void sendWarningToUser(const AnsiWarningMessage &warning);
    void sendErrorToUser(std::string_view msg);
    void sendStatusToUser(std::string_view msg);
    void sendSyntaxHintToUser(std::string_view before, std::string_view cmd, std::string_view after);

private:
    void onSendToMudSocket(const TelnetIacBytes &);

private:
    NODISCARD Pipeline &getPipeline() { return deref(m_pipeline); }

    NODISCARD GameObserver &getGameObserver() { return m_gameObserver; }
    NODISCARD MainWindow &getMainWindow() { return m_mainWindow; }
    NODISCARD MapCanvas &getMapCanvas() { return m_mapCanvas; }
    NODISCARD Mmapper2Group &getGroupManager() { return m_groupManager; }
    NODISCARD MumeClock &getMumeClock() { return m_mumeClock; }
    NODISCARD Mmapper2PathMachine &getPathMachine() { return m_pathMachine; }
    NODISCARD PrespammedPath &getPrespam() { return m_prespammedPath; }

    NODISCARD MumeFallbackSocket &getMudSocket() { return deref(getPipeline().mud.mudSocket); }
    NODISCARD MudTelnet &getMudTelnet() { return deref(getPipeline().mud.mudTelnet); }
    NODISCARD TelnetLineFilter &getMudTelnetFilter()
    {
        return deref(getPipeline().mud.mudTelnetFilter);
    }
    NODISCARD MpiFilter &getMpiFilterFromMud() { return deref(getPipeline().mud.mpiFilterFromMud); }
    NODISCARD MpiFilterToMud &getMpiFilterToMud()
    {
        return deref(getPipeline().mud.mpiFilterToMud);
    }

    NODISCARD UserSocket &getUserSocket() { return deref(getPipeline().user.userSocket); }
    NODISCARD UserTelnet &getUserTelnet() { return deref(getPipeline().user.userTelnet); }
    NODISCARD TelnetLineFilter &getUserTelnetFilter()
    {
        return deref(getPipeline().user.userTelnetFilter);
    }
    NODISCARD RemoteEdit &getRemoteEdit();
    NODISCARD MumeXmlParser &getMudParser() { return deref(getPipeline().mud.mudParser); }
    NODISCARD AbstractParser &getUserParser() { return deref(getPipeline().user.userParser); }
    NODISCARD PasswordConfig &getPasswordConfig()
    {
        return deref(getPipeline().mud.passwordConfig);
    }

private:
    NODISCARD const Pipeline &getPipeline() const { return deref(m_pipeline); }
    NODISCARD const MudTelnet &getMudTelnet() const { return deref(getPipeline().mud.mudTelnet); }
    NODISCARD const UserTelnet &getUserTelnet() const
    {
        return deref(getPipeline().user.userTelnet);
    }

private:
    NODISCARD bool hasConnectedUserSocket() const;
};
