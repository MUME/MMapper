// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "proxy.h"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QScopedPointer>
#include <QSslSocket>
#include <QTcpSocket>

#include "../clock/mumeclock.h"
#include "../configuration/configuration.h"
#include "../display/mapcanvas.h"
#include "../display/prespammedpath.h"
#include "../expandoracommon/parseevent.h"
#include "../global/io.h"
#include "../mainwindow/mainwindow.h"
#include "../mpi/mpifilter.h"
#include "../mpi/remoteedit.h"
#include "../pandoragroup/mmapper2group.h"
#include "../parser/abstractparser.h"
#include "../parser/mumexmlparser.h"
#include "../pathmachine/mmapper2pathmachine.h"
#include "../roompanel/RoomManager.h"
#include "MudTelnet.h"
#include "UserTelnet.h"
#include "connectionlistener.h"
#include "mumesocket.h"
#include "telnetfilter.h"

#undef ERROR // Bad dog, Microsoft; bad dog!!!

template<typename T, typename... Args>
NODISCARD static inline auto makeQPointer(Args &&...args)
{
    static_assert(std::is_base_of_v<QObject, T>);
    auto tmp = std::make_unique<T>(std::forward<Args>(args)...);
    if (tmp->QObject::parent() == nullptr)
        throw std::invalid_argument("allocated QObject does not have a parent");
    return QPointer<T>{tmp.release()};
}

Proxy::Proxy(MapData &md,
             Mmapper2PathMachine &pm,
             PrespammedPath &pp,
             Mmapper2Group &gm,
             MumeClock &mc,
             MapCanvas &mca,
             GameObserver &go,
             qintptr &socketDescriptor,
             ConnectionListener &listener)
    : QObject(nullptr)
    , m_mapData(md)
    , m_pathMachine(pm)
    , m_prespammedPath(pp)
    , m_groupManager(gm)
    , m_mumeClock(mc)
    , m_mapCanvas(mca)
    , m_gameObserver(go)
    , m_socketDescriptor(socketDescriptor)
    , m_listener(listener)
    // TODO: pass this in as a non-owning pointer.
    , m_remoteEdit{makeQPointer<RemoteEdit>(m_listener.parent())}
{
    //
}

Proxy::~Proxy()
{
    if (auto userSocket = m_userSocket.data()) {
        userSocket->flush();
        userSocket->disconnectFromHost();
    }

    if (auto mudSocket = m_mudSocket.data()) {
        mudSocket->disconnectFromHost();
    }

    if (auto remoteEdit = m_remoteEdit.data()) {
        remoteEdit->deleteLater(); // Ugg.
    }
}

void Proxy::slot_start()
{
    auto *const mw = dynamic_cast<MainWindow *>(m_listener.parent());
    if (mw == nullptr) {
        // dynamic cast can fail
        assert(false);
        return;
    }

    m_userSocket = [this]() -> QPointer<QTcpSocket> {
        auto userSock = makeQPointer<QTcpSocket>(this);
        if (!userSock->setSocketDescriptor(m_socketDescriptor)) {
            return {};
        }
        userSock->setSocketOption(QAbstractSocket::LowDelayOption, true);
        userSock->setSocketOption(QAbstractSocket::KeepAliveOption, true);
        return userSock;
    }();

    if (!m_userSocket) {
        deleteLater();
        // REVISIT: Under what conditions can this happen? This seems like a VERY serious failure,
        // so shouldn't we throw here instead of returning like everything's okay?
        return;
    }

    m_userTelnet = makeQPointer<UserTelnet>(this);
    m_mudTelnet = makeQPointer<MudTelnet>(this);
    m_telnetFilter = makeQPointer<TelnetFilter>(this);
    m_mpiFilter = makeQPointer<MpiFilter>(this);
    m_timers = makeQPointer<CTimers>(this);
    m_parserXml = makeQPointer<MumeXmlParser>(m_mapData,
                                              m_mumeClock,
                                              m_proxyParserApi,
                                              m_groupManager.getGroupManagerApi(),
                                              *m_timers,
                                              this);

    // REVISIT: The mud socket is never altered if the user changes socket settings
    m_mudSocket = [this] {
        const auto &conf = getConfig().connection;
        if (!NO_WEBSOCKETS && conf.webSocket)
            return QPointer<MumeSocket>(makeQPointer<MumeWebSocket>(this));
        if (!QSslSocket::supportsSsl() && conf.tlsEncryption)
            return QPointer<MumeSocket>(makeQPointer<MumeSslSocket>(this));
        return QPointer<MumeSocket>(makeQPointer<MumeTcpSocket>(this));
    }();

    auto *const userSocket = m_userSocket.data();
    auto *const userTelnet = m_userTelnet.data();
    auto *const mudTelnet = m_mudTelnet.data();
    auto *const telnetFilter = m_telnetFilter.data();
    auto *const mpiFilter = m_mpiFilter.data();
    auto *const parserXml = m_parserXml.data();
    auto *const mudSocket = m_mudSocket.data();
    auto *const remoteEdit = m_remoteEdit.data();

    connect(this, &Proxy::sig_log, mw, &MainWindow::slot_log);
    connect(this, &Proxy::sig_sendToMud, mudTelnet, &MudTelnet::slot_onSendToMud);
    connect(this, &Proxy::sig_sendToUser, userTelnet, &UserTelnet::slot_onSendToUser);
    connect(this, &Proxy::sig_gmcpToMud, mudTelnet, &MudTelnet::slot_onGmcpToMud);
    connect(this, &Proxy::sig_gmcpToUser, userTelnet, &UserTelnet::slot_onGmcpToUser);
    connect(userSocket, &QAbstractSocket::disconnected, this, &Proxy::slot_userTerminatedConnection);
    connect(userSocket, &QIODevice::readyRead, this, &Proxy::slot_processUserStream);

    connect(userTelnet,
            &UserTelnet::sig_analyzeUserStream,
            telnetFilter,
            &TelnetFilter::slot_onAnalyzeUserStream);
    connect(userTelnet, &UserTelnet::sig_sendToSocket, this, &Proxy::slot_onSendToUserSocket);
    connect(userTelnet, &UserTelnet::sig_relayGmcp, mudTelnet, &MudTelnet::slot_onGmcpToMud);
    connect(userTelnet, &UserTelnet::sig_relayNaws, mudTelnet, &MudTelnet::slot_onRelayNaws);
    connect(userTelnet, &UserTelnet::sig_relayTermType, mudTelnet, &MudTelnet::slot_onRelayTermType);

    connect(mudTelnet,
            &MudTelnet::sig_analyzeMudStream,
            telnetFilter,
            &TelnetFilter::slot_onAnalyzeMudStream);
    connect(mudTelnet, &MudTelnet::sig_sendToSocket, this, &Proxy::slot_onSendToMudSocket);
    connect(mudTelnet, &MudTelnet::sig_relayEchoMode, userTelnet, &UserTelnet::slot_onRelayEchoMode);
    connect(mudTelnet, &MudTelnet::sig_relayGmcp, userTelnet, &UserTelnet::slot_onGmcpToUser);
    connect(mudTelnet,
            &MudTelnet::sig_relayGmcp,
            &m_groupManager,
            &Mmapper2Group::slot_parseGmcpInput);
    connect(mudTelnet, &MudTelnet::sig_relayGmcp, m_parserXml, &MumeXmlParser::slot_parseGmcpInput);

    connect(this, &Proxy::sig_analyzeUserStream, userTelnet, &UserTelnet::slot_onAnalyzeUserStream);

    connect(telnetFilter,
            &TelnetFilter::sig_parseNewMudInput,
            mpiFilter,
            &MpiFilter::slot_analyzeNewMudInput);
    connect(mpiFilter, &MpiFilter::sig_sendToMud, mudTelnet, &MudTelnet::slot_onSendToMud);
    connect(mpiFilter, &MpiFilter::sig_editMessage, remoteEdit, &RemoteEdit::slot_remoteEdit);
    connect(mpiFilter, &MpiFilter::sig_viewMessage, remoteEdit, &RemoteEdit::slot_remoteView);
    connect(remoteEdit, &RemoteEdit::sig_sendToSocket, mudTelnet, &MudTelnet::slot_onSendToMud);

    connect(mpiFilter,
            &MpiFilter::sig_parseNewMudInput,
            parserXml,
            &MumeXmlParser::slot_parseNewMudInput);
    connect(telnetFilter,
            &TelnetFilter::sig_parseNewUserInput,
            parserXml,
            &MumeXmlParser::slot_parseNewUserInput);

    connect(parserXml, &MumeXmlParser::sig_sendToMud, mudTelnet, &MudTelnet::slot_onSendToMud);
    connect(parserXml, &MumeXmlParser::sig_sendToUser, userTelnet, &UserTelnet::slot_onSendToUser);

    connect(parserXml,
            &MumeXmlParser::sig_handleParseEvent,
            &m_pathMachine,
            &Mmapper2PathMachine::slot_handleParseEvent);
    connect(parserXml,
            &AbstractParser::sig_releaseAllPaths,
            &m_pathMachine,
            &PathMachine::slot_releaseAllPaths);
    connect(parserXml,
            &AbstractParser::sig_showPath,
            &m_prespammedPath,
            &PrespammedPath::slot_setPath);
    connect(parserXml, &AbstractParser::sig_mapChanged, &m_mapCanvas, &MapCanvas::mapChanged);
    connect(parserXml,
            &AbstractParser::sig_graphicsSettingsChanged,
            &m_mapCanvas,
            &MapCanvas::graphicsSettingsChanged);
    connect(parserXml, &AbstractParser::sig_log, mw, &MainWindow::slot_log);
    connect(parserXml,
            &AbstractParser::sig_newRoomSelection,
            &m_mapCanvas,
            &MapCanvas::slot_setRoomSelection);

    connect(userSocket, &QAbstractSocket::disconnected, parserXml, &AbstractParser::slot_reset);

    // Group Manager Support
    connect(parserXml, &AbstractParser::sig_showPath, &m_groupManager, &Mmapper2Group::slot_setPath);

    // timers
    connect(m_timers,
            &CTimers::sig_sendTimersUpdateToUser,
            parserXml,
            &AbstractParser::slot_timersUpdate);

    // Group Tell
    connect(&m_groupManager,
            &Mmapper2Group::sig_displayGroupTellEvent,
            parserXml,
            &AbstractParser::slot_sendGTellToUser);

    // Game Observer (re-broadcasts text and gmcp updates to downstream consumers)
    connect(mudSocket,
            &MumeSocket::sig_connected,
            &m_gameObserver,
            &GameObserver::slot_observeConnected);
    connect(parserXml,
            &MumeXmlParser::sig_sendToMud,
            &m_gameObserver,
            &GameObserver::slot_observeSentToMud);
    connect(parserXml,
            &MumeXmlParser::sig_sendToUser,
            &m_gameObserver,
            &GameObserver::slot_observeSentToUser);
    // note the polarity, unlike above: MudTelnet::relay is SentToUser, UserTelnet::relay is SentToMud
    connect(mudTelnet,
            &MudTelnet::sig_relayGmcp,
            &m_gameObserver,
            &GameObserver::slot_observeSentToUserGmcp);
    connect(userTelnet,
            &UserTelnet::sig_relayGmcp,
            &m_gameObserver,
            &GameObserver::slot_observeSentToMudGmcp);
    connect(mudTelnet,
            &MudTelnet::sig_relayEchoMode,
            &m_gameObserver,
            &GameObserver::slot_observeToggledEchoMode);

    log("Connection to client established ...");

    QByteArray ba = QString("\033[0;1;37;46m"
                            "Welcome to MMapper!"
                            "\033[0;37;46m"
                            "   Type "
                            "\033[1m"
                            "%1help"
                            "\033[0;37;46m"
                            " for help."
                            "\033[0m"
                            "\n")
                        .arg(getConfig().parser.prefixChar)
                        .toLatin1();
    sendToUser(ba);

    connect(mudSocket, &MumeSocket::sig_connected, userTelnet, &UserTelnet::slot_onConnected);
    connect(mudSocket, &MumeSocket::sig_connected, this, &Proxy::slot_onMudConnected);
    connect(mudSocket, &MumeSocket::sig_socketError, parserXml, &AbstractParser::slot_reset);
    connect(mudSocket, &MumeSocket::sig_socketError, &m_groupManager, &Mmapper2Group::slot_reset);
    connect(mudSocket, &MumeSocket::sig_socketError, this, &Proxy::slot_onMudError);
    connect(mudSocket, &MumeSocket::sig_disconnected, mudTelnet, &MudTelnet::slot_onDisconnected);
    connect(mudSocket, &MumeSocket::sig_disconnected, parserXml, &AbstractParser::slot_reset);
    connect(mudSocket, &MumeSocket::sig_disconnected, &m_groupManager, &Mmapper2Group::slot_reset);
    connect(mudSocket, &MumeSocket::sig_disconnected, this, &Proxy::slot_mudTerminatedConnection);
    connect(mudSocket,
            &MumeSocket::sig_disconnected,
            m_remoteEdit,
            &RemoteEdit::slot_onDisconnected);
    connect(mudSocket,
            &MumeSocket::sig_processMudStream,
            mudTelnet,
            &MudTelnet::slot_onAnalyzeMudStream);
    connect(mudSocket, &MumeSocket::sig_log, mw, &MainWindow::slot_log);

    // connect signals emitted from user commands to change mode;
    // they are connected here because proxy knows about both the parser
    // (within which commands are handled) and the mainwindow, where the mode
    // state is stored.
    connect(parserXml, &AbstractParser::sig_setMode, mw, &MainWindow::slot_setMode);

    // emitted after modifying infomarks for _infomark command
    connect(parserXml,
            &AbstractParser::sig_infoMarksChanged,
            &m_mapCanvas,
            &MapCanvas::infomarksChanged);

    connectToMud();
}

void Proxy::slot_onMudConnected()
{
    const auto &settings = getConfig().mumeClientProtocol;

    m_serverState = ServerStateEnum::CONNECTED;

    log("Connection to server established ...");

    // send gratuitous XML request
    sendToMud(QByteArray("~$#EX2\n3G\n"));
    log("Sent MUME Protocol Initiator XML request");

    // send Remote Editing request
    if (settings.remoteEditing) {
        sendToMud(QByteArray("~$#EI\n"));
        log("Sent MUME Protocol Initiator remote editing request");
    }

    // Reset clock precision to its lowest level
    m_mumeClock.setPrecision(MumeClockPrecisionEnum::UNSET);
}

void Proxy::slot_onMudError(const QString &errorStr)
{
    m_serverState = ServerStateEnum::ERROR;

    qWarning() << "Mud socket error" << errorStr;
    log(errorStr);

    sendToUser("\n"
               "\033[0;37;46m"
               + errorStr.toLocal8Bit()
               + "\033[0m"
                 "\n");

    if (!getConfig().connection.proxyConnectionStatus) {
        sendToUser(QString("\n"
                           "\033[0;37;46m"
                           "You can type "
                           "\033[1m"
                           "%1connect"
                           "\033[0m\033[37;46m"
                           " to reconnect again."
                           "\033[0m"
                           "\n")
                       .arg(getConfig().parser.prefixChar)
                       .toLatin1());
        m_parserXml->sendPromptToUser();
        m_serverState = ServerStateEnum::OFFLINE;
    } else if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
        sendToUser("\n"
                   "\033[0;37;46m"
                   "You are now exploring the map offline."
                   "\033[0m"
                   "\n");
        m_parserXml->sendPromptToUser();
        m_serverState = ServerStateEnum::OFFLINE;
    } else {
        // Terminate connection
        deleteLater();
    }
}

void Proxy::slot_userTerminatedConnection()
{
    log("User terminated connection ...");
    deleteLater();
}

void Proxy::slot_mudTerminatedConnection()
{
    if (!isConnected()) {
        return;
    }

    m_serverState = ServerStateEnum::DISCONNECTED;

    m_userTelnet->slot_onRelayEchoMode(true);

    log("Mud terminated connection ...");

    sendToUser("\n"
               "\033[0;37;46m"
               "MUME closed the connection."
               "\033[0m"
               "\n");

    if (getConfig().connection.proxyConnectionStatus) {
        if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
            sendToUser("\n"
                       "\033[0;37;46m"
                       "You are now exploring the map offline."
                       "\033[0m"
                       "\n");
            m_parserXml->sendPromptToUser();
        } else {
            // Terminate connection
            deleteLater();
        }
    } else {
        sendToUser(QString("\n"
                           "\033[0;37;46m"
                           "You can type "
                           "\033[1m"
                           "%1connect"
                           "\033[0;37;46m"
                           " to reconnect again."
                           "\033[0m"
                           "\n")
                       .arg(getConfig().parser.prefixChar)
                       .toLatin1());
        m_parserXml->sendPromptToUser();
    }
}

void Proxy::slot_processUserStream()
{
    if (m_userSocket == nullptr)
        return;

    // REVISIT: check return value?
    MAYBE_UNUSED const auto ignored = //
        io::readAllAvailable(*m_userSocket, m_buffer, [this](const QByteArray &byteArray) {
            if (!byteArray.isEmpty())
                emit sig_analyzeUserStream(byteArray);
        });
}

void Proxy::slot_onSendToMudSocket(const QByteArray &ba)
{
    if (m_mudSocket != nullptr) {
        if (m_mudSocket->state() != QAbstractSocket::ConnectedState) {
            sendToUser(QString("\033[0;37;46m"
                               "MMapper is not connected to MUME. Please type "
                               "\033[1m"
                               "%1connect"
                               "\033[0;37;46m"
                               " to play."
                               "\033[0m"
                               "\n")
                           .arg(getConfig().parser.prefixChar)
                           .toLatin1());

            m_parserXml->sendPromptToUser();
        } else {
            m_mudSocket->sendToMud(ba);
        }
    } else {
        qWarning() << "Mud socket not available";
    }
}

void Proxy::slot_onSendToUserSocket(const QByteArray &ba)
{
    if (m_userSocket != nullptr) {
        m_userSocket->write(ba);
    } else {
        qWarning() << "User socket not available";
    }
}

bool Proxy::isConnected() const
{
    return m_serverState == ServerStateEnum::CONNECTED;
}

void Proxy::connectToMud()
{
    switch (m_serverState) {
    case ServerStateEnum::CONNECTING:
        sendToUser("Error: You're still connecting.\n");
        break;

    case ServerStateEnum::CONNECTED:
        sendToUser("Error: You're already connected.\n");
        break;

    case ServerStateEnum::DISCONNECTING:
        sendToUser("Error: You're still disconnecting.\n");
        break;

    case ServerStateEnum::INITIALIZED:
    case ServerStateEnum::OFFLINE:
    case ServerStateEnum::DISCONNECTED:
    case ServerStateEnum::ERROR: {
        if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
            sendToUser(
                "\n"
                "\033[37;46m"
                "MMapper is running in offline mode. Switch modes and reconnect to play MUME."
                "\033[0m"
                "\n"
                "\n"
                "Welcome to the land of Middle-earth. May your visit here be... interesting.\n"
                "Never forget! Try to role-play...\n");
            m_parserXml->doMove(CommandEnum::LOOK);
            m_serverState = ServerStateEnum::OFFLINE;
            break;
        }

        if (auto sock = m_mudSocket.data()) {
            sendToUser("Connecting...\n");
            m_serverState = ServerStateEnum::CONNECTING;
            sock->connectToHost();
        } else {
            sendToUser("Internal error while trying to connect.\n");
        }
        break;
    }
    }
}

void Proxy::disconnectFromMud()
{
    m_userTelnet->slot_onRelayEchoMode(true);

    switch (m_serverState) {
    case ServerStateEnum::CONNECTING:
        sendToUser("Error: You're still connecting.\n");
        break;

    case ServerStateEnum::OFFLINE:
        m_serverState = ServerStateEnum::INITIALIZED;
        sendToUser("You disconnect your simulated link.\n");
        break;

    case ServerStateEnum::CONNECTED: {
        if (auto sock = m_mudSocket.data()) {
            sendToUser("Disconnecting...\n");
            m_serverState = ServerStateEnum::DISCONNECTING;
            sock->disconnectFromHost();
            sendToUser("Disconnected.\n");
            m_serverState = ServerStateEnum::DISCONNECTED;
        } else {
            sendToUser("Internal error while trying to disconnect.\n");
        }
        break;
    }

    case ServerStateEnum::DISCONNECTING:
        sendToUser("Error: You're still disconnecting.\n");
        break;

    case ServerStateEnum::INITIALIZED:
    case ServerStateEnum::DISCONNECTED:
    case ServerStateEnum::ERROR:
        sendToUser("Error: You're not connected.\n");
        break;
    }
}

bool Proxy::isGmcpModuleEnabled(const GmcpModuleTypeEnum &module) const
{
    return m_userTelnet->isGmcpModuleEnabled(module);
}
