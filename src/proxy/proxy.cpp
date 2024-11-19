// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "proxy.h"

#include "../clock/mumeclock.h"
#include "../configuration/configuration.h"
#include "../display/mapcanvas.h"
#include "../display/prespammedpath.h"
#include "../global/MakeQPointer.h"
#include "../global/SendToUser.h"
#include "../global/io.h"
#include "../mainwindow/mainwindow.h"
#include "../map/parseevent.h"
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

#include <cassert>
#include <memory>
#include <stdexcept>
#include <tuple>

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QScopedPointer>
#include <QSslSocket>
#include <QTcpSocket>

using mmqt::makeQPointer;

namespace { // anonymous
constexpr const auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);
constexpr const auto boldWhiteOnCyan = whiteOnCyan.withBold();
} // namespace

Proxy::Proxy(MapData &md,
             Mmapper2PathMachine &pm,
             PrespammedPath &pp,
             Mmapper2Group &gm,
             MumeClock &mc,
             MapCanvas &mca,
             GameObserver &go,
             qintptr &socketDescriptor,
             ConnectionListener &listener)
    : QObject(&listener) // parent must be set in order to use mmqt::makeQPointer<Proxy>()
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
    m_userTelnetFilter = makeQPointer<UserTelnetFilter>(this);
    m_mudTelnetFilter = makeQPointer<MudTelnetFilter>(this);
    m_mpiFilter = makeQPointer<MpiFilter>(this);
    m_timers = makeQPointer<CTimers>(this);
    m_parserXml = makeQPointer<MumeXmlParser>(m_mapData,
                                              m_mumeClock,
                                              m_proxyParserApi,
                                              m_groupManager.getGroupManagerApi(),
                                              *m_timers,
                                              this);

    m_mudSocket = (!QSslSocket::supportsSsl() || !getConfig().connection.tlsEncryption)
                      ? QPointer<MumeSocket>(makeQPointer<MumeTcpSocket>(this))
                      : QPointer<MumeSocket>(makeQPointer<MumeSslSocket>(this));

    auto *const userSocket = m_userSocket.data();
    auto *const userTelnet = m_userTelnet.data();
    auto *const mudTelnet = m_mudTelnet.data();
    auto *const userTelnetFilter = m_userTelnetFilter.data();
    auto *const mudTelnetFilter = m_mudTelnetFilter.data();
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
            userTelnetFilter,
            &UserTelnetFilter::slot_onAnalyzeUserStream);
    connect(userTelnet, &UserTelnet::sig_sendToSocket, this, &Proxy::slot_onSendToUserSocket);
    connect(userTelnet, &UserTelnet::sig_relayGmcp, mudTelnet, &MudTelnet::slot_onGmcpToMud);
    connect(userTelnet, &UserTelnet::sig_relayNaws, mudTelnet, &MudTelnet::slot_onRelayNaws);
    connect(userTelnet, &UserTelnet::sig_relayTermType, mudTelnet, &MudTelnet::slot_onRelayTermType);

    connect(mudTelnet,
            &MudTelnet::sig_analyzeMudStream,
            mudTelnetFilter,
            &MudTelnetFilter::slot_onAnalyzeMudStream);
    connect(mudTelnet, &MudTelnet::sig_sendToSocket, this, &Proxy::slot_onSendToMudSocket);
    connect(mudTelnet, &MudTelnet::sig_relayEchoMode, userTelnet, &UserTelnet::slot_onRelayEchoMode);
    connect(mudTelnet, &MudTelnet::sig_relayGmcp, userTelnet, &UserTelnet::slot_onGmcpToUser);
    connect(mudTelnet,
            &MudTelnet::sig_sendGameTimeToClock,
            this,
            &Proxy::slot_onSendGameTimeToClock);
    connect(mudTelnet,
            &MudTelnet::sig_sendMSSPToUser,
            userTelnet,
            &UserTelnet::slot_onSendMSSPToUser);

    connect(mudTelnet,
            &MudTelnet::sig_relayGmcp,
            &m_groupManager,
            &Mmapper2Group::slot_parseGmcpInput);
    connect(mudTelnet, &MudTelnet::sig_relayGmcp, m_parserXml, &MumeXmlParser::slot_parseGmcpInput);

    connect(this, &Proxy::sig_analyzeUserStream, userTelnet, &UserTelnet::slot_onAnalyzeUserStream);

    connect(mudTelnetFilter,
            &MudTelnetFilter::sig_parseNewMudInput,
            mpiFilter,
            &MpiFilter::slot_analyzeNewMudInput);
    connect(mpiFilter, &MpiFilter::sig_editMessage, remoteEdit, &RemoteEdit::slot_remoteEdit);
    connect(mpiFilter, &MpiFilter::sig_viewMessage, remoteEdit, &RemoteEdit::slot_remoteView);
    connect(remoteEdit, &RemoteEdit::sig_sendToSocket, mudTelnet, &MudTelnet::slot_onSendRawToMud);

    connect(mpiFilter,
            &MpiFilter::sig_parseNewMudInput,
            parserXml,
            &MumeXmlParser::slot_parseNewMudInput);
    connect(userTelnetFilter,
            &UserTelnetFilter::sig_parseNewUserInput,
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
    connect(parserXml, &AbstractParser::sig_mapChanged, &m_mapCanvas, &MapCanvas::slot_mapChanged);
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

    connect(&m_mapData,
            &MapData::sig_onForcedPositionChange,
            parserXml,
            &AbstractParser::slot_onForcedPositionChange);

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
    connect(mudSocket, &MumeSocket::sig_connected, this, [this]() {
        m_gameObserver.slot_observeConnected();
    });
    connect(parserXml, &MumeXmlParser::sig_sendToMud, this, [this](const QString &msg) {
        m_gameObserver.slot_observeSentToMud(msg);
    });
    connect(parserXml,
            &MumeXmlParser::sig_sendToUser,
            this,
            [this](const QString &msg, auto goAhead) {
                m_gameObserver.slot_observeSentToUser(msg, goAhead);
            });
    // note the polarity, unlike above: MudTelnet::relay is SentToUser, UserTelnet::relay is SentToMud
    connect(mudTelnet, &MudTelnet::sig_relayGmcp, this, [this](const GmcpMessage &msg) {
        m_gameObserver.slot_observeSentToUserGmcp(msg);
    });
    connect(userTelnet, &UserTelnet::sig_relayGmcp, this, [this](const GmcpMessage &msg) {
        m_gameObserver.slot_observeSentToMudGmcp(msg);
    });
    connect(mudTelnet, &MudTelnet::sig_relayEchoMode, this, [this](const bool echo) {
        m_gameObserver.slot_observeToggledEchoMode(echo);
    });

    global::registerSendToUser(m_lifetime, [this](const QString &str) {
        // REVISIT: We don't know if we're at a prompt or not.
        sendToUser("\n");
        sendToUser(str);
        deref(m_parserXml).sendPromptToUser();
    });

    log("Connection to client established ...");
    sendWelcomeToUser();
    sendSyntaxHintToUser("Type", "help", "for help.");

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
            &MapCanvas::slot_infomarksChanged);

    connectToMud();
}

void Proxy::slot_onMudConnected()
{
    const auto &settings = getConfig().mumeClientProtocol;

    m_serverState = ServerStateEnum::Connected;

    log("Connection to server established ...");

    // send gratuitous XML request
    sendToMud("~$#EX2\n3G\n");
    log("Sent MUME Protocol Initiator XML request");

    // send Remote Editing request
    if (settings.remoteEditing) {
        sendToMud("~$#EI\n");
        log("Sent MUME Protocol Initiator remote editing request");
    }

    // Reset clock precision to its lowest level
    m_mumeClock.setPrecision(MumeClockPrecisionEnum::UNSET);
}

void Proxy::slot_onMudError(const QString &errorStr)
{
    m_serverState = ServerStateEnum::Error;

    qWarning() << "Mud socket error" << errorStr;
    log(errorStr);

    sendNewlineToUser();
    sendErrorToUser(mmqt::toStdStringUtf8(errorStr));

    if (!getConfig().connection.proxyConnectionStatus) {
        sendNewlineToUser();
        sendSyntaxHintToUser("You can type", "connect", "to reconnect again.");
        m_parserXml->sendPromptToUser();
        m_serverState = ServerStateEnum::Offline;
    } else if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
        sendNewlineToUser();
        sendStatusToUser("You are now exploring the map offline.");
        m_parserXml->sendPromptToUser();
        m_serverState = ServerStateEnum::Offline;
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

    m_serverState = ServerStateEnum::Disconnected;

    m_userTelnet->slot_onRelayEchoMode(true);

    log("Mud terminated connection ...");

    sendNewlineToUser();
    sendStatusToUser("MUME closed the connection.");

    if (!getConfig().connection.proxyConnectionStatus) {
        sendNewlineToUser();
        sendSyntaxHintToUser("You can type", "connect", "to reconnect again.");
        m_parserXml->sendPromptToUser();
    } else if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
        sendNewlineToUser();
        sendStatusToUser("You are now exploring the map offline.");
        m_parserXml->sendPromptToUser();
    } else {
        // Terminate connection
        deleteLater();
    }
}

void Proxy::slot_processUserStream()
{
    if (m_userSocket == nullptr) {
        return;
    }

    // REVISIT: check return value?
    std::ignore = io::readAllAvailable(*m_userSocket, m_buffer, [this](const QByteArray &byteArray) {
        assert(!byteArray.isEmpty());
        emit sig_analyzeUserStream(TelnetIacBytes{byteArray});
    });
}

void Proxy::slot_onSendToMudSocket(const TelnetIacBytes &s)
{
    if (m_mudSocket == nullptr) {
        qWarning() << "Mud socket not available";
        return;
    }

    if (!m_mudSocket->isConnectedOrConnecting()) {
        sendStatusToUser("MMapper is not connected to MUME.");
        sendSyntaxHintToUser("You can type", "connect", "to play.");
        m_parserXml->sendPromptToUser();
        return;
    }

    m_mudSocket->sendToMud(s);
}

void Proxy::slot_onSendToUserSocket(const TelnetIacBytes &s)
{
    if (m_userSocket == nullptr) {
        qWarning() << "User socket not available";
        return;
    }

    m_userSocket->write(s.getQByteArray());
}

bool Proxy::isConnected() const
{
    return m_serverState == ServerStateEnum::Connected;
}

void Proxy::connectToMud()
{
    switch (m_serverState) {
    case ServerStateEnum::Connecting:
        sendErrorToUser("You're still connecting.");
        break;

    case ServerStateEnum::Connected:
        sendErrorToUser("You're already connected.");
        break;

    case ServerStateEnum::Disconnecting:
        sendErrorToUser("You're still disconnecting.");
        break;

    case ServerStateEnum::Initialized:
    case ServerStateEnum::Offline:
    case ServerStateEnum::Disconnected:
    case ServerStateEnum::Error: {
        if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
            sendNewlineToUser();
            sendStatusToUser("MMapper is running in offline mode.");
            sendSyntaxHintToUser("Switch modes and", "connect", "to play MUME.");
            sendToUser(
                "\n\n"
                "Welcome to the land of Middle-earth. May your visit here be... interesting.\n"
                "Never forget! Try to role-play...\n");
            m_parserXml->doMove(CommandEnum::LOOK);
            m_serverState = ServerStateEnum::Offline;
            break;
        }

        if (auto sock = m_mudSocket.data()) {
            sendStatusToUser("Connecting...");
            m_serverState = ServerStateEnum::Connecting;
            sock->connectToHost();
        } else {
            sendErrorToUser("Internal error while trying to connect.");
        }
        break;
    }
    }
}

void Proxy::disconnectFromMud()
{
    m_userTelnet->slot_onRelayEchoMode(true);

    switch (m_serverState) {
    case ServerStateEnum::Connecting:
        sendErrorToUser("You're still connecting.");
        break;

    case ServerStateEnum::Offline:
        m_serverState = ServerStateEnum::Initialized;
        sendStatusToUser("You disconnect your simulated link.");
        break;

    case ServerStateEnum::Connected: {
        if (auto sock = m_mudSocket.data()) {
            sendStatusToUser("Disconnecting...");
            m_serverState = ServerStateEnum::Disconnecting;
            sock->disconnectFromHost();
            sendStatusToUser("Disconnected.");
            m_serverState = ServerStateEnum::Disconnected;
        } else {
            sendErrorToUser("Internal error while trying to disconnect.");
        }
        break;
    }

    case ServerStateEnum::Disconnecting:
        sendErrorToUser("You're still disconnecting.");
        break;

    case ServerStateEnum::Initialized:
    case ServerStateEnum::Disconnected:
    case ServerStateEnum::Error:
        sendErrorToUser("You're not connected.");
        break;
    }
}

bool Proxy::isGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const
{
    return m_userTelnet->isGmcpModuleEnabled(mod);
}

void Proxy::slot_onSendGameTimeToClock(const int year,
                                       const std::string &monthStr,
                                       const int day,
                                       const int hour)
{
    // Month from MSSP comes as a string, so fetch the month index.
    const int month = MumeClock::getMumeMonth(mmqt::toQStringUtf8(monthStr));
    m_mumeClock.parseMSSP(year, month, day, hour);
}

auto Proxy::getSendToUserAnsiOstream() -> AnsiHelper
{
    return AnsiHelper{m_lifetime,
                      [this](const std::string &s) -> void { sendToUser(mmqt::toQStringUtf8(s)); }};
}

void Proxy::sendWelcomeToUser()
{
    auto aos = getSendToUserAnsiOstream();
    aos.writeWithColor(boldWhiteOnCyan, "=== Welcome to MMapper! ===");
    aos.write("\n");
}

void Proxy::sendErrorToUser(const std::string_view msg)
{
    auto aos = getSendToUserAnsiOstream();
    aos.writeWithColor(boldWhiteOnCyan, "Error");
    aos.writeWithColor(whiteOnCyan, ": ");
    aos.writeWithColor(whiteOnCyan, msg);
    aos.write("\n");
}

void Proxy::sendStatusToUser(const std::string_view msg)
{
    auto aos = getSendToUserAnsiOstream();
    if (true) {
        aos.writeWithColor(boldWhiteOnCyan, "Status");
        aos.writeWithColor(whiteOnCyan, ": ");
    }
    aos.writeWithColor(whiteOnCyan, msg);
    aos.write("\n");
}

void Proxy::sendSyntaxHintToUser(const std::string_view before,
                                 const std::string_view cmd,
                                 const std::string_view after)
{
    auto aos = getSendToUserAnsiOstream();
    if (true) {
        aos.writeWithColor(boldWhiteOnCyan, "Hint");
        aos.writeWithColor(whiteOnCyan, ": ");
    }
    aos.writeWithColor(whiteOnCyan, before);
    aos.writeWithColor(whiteOnCyan, " ");
    aos.writeWithColor(boldWhiteOnCyan, getConfig().parser.prefixChar);
    aos.writeWithColor(boldWhiteOnCyan, cmd);
    aos.writeWithColor(whiteOnCyan, " ");
    aos.writeWithColor(whiteOnCyan, after);
    aos.write("\n");
}
