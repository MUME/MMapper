// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "proxy.h"

#include "../clock/mumeclock.h"
#include "../configuration/PasswordConfig.h"
#include "../configuration/configuration.h"
#include "../display/mapcanvas.h"
#include "../display/prespammedpath.h"
#include "../global/LineUtils.h"
#include "../global/MakeQPointer.h"
#include "../global/SendToUser.h"
#include "../global/Version.h"
#include "../global/io.h"
#include "../group/mmapper2group.h"
#include "../mainwindow/mainwindow.h"
#include "../map/parseevent.h"
#include "../mpi/mpifilter.h"
#include "../mpi/remoteedit.h"
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

// If this is true, then "Error: xxx", "Hint: xxx", or "Status: xxx" is shown
// otherwise if false, then just only "xxx" is shown.
const volatile bool g_prefixMessagesToUser = true;
const volatile bool g_showVersionInWelcomeMessage = IS_DEBUG_BUILD;
//
constexpr const auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);

NODISCARD MainWindow &getMainWindow(ConnectionListener &listener)
{
    // dynamic cast can fail
    auto *const mw = dynamic_cast<MainWindow *>(listener.parent());
    if (mw == nullptr) {
        throw std::runtime_error("ConnectionListener's parent must be MainWindow");
    }
    return *mw;
}

} // namespace

struct NODISCARD Proxy::UserSocketOutputs
{
public:
    virtual ~UserSocketOutputs();

public:
    void onDisconnected() { virt_onDisconnected(); }
    void onReadyRead() { virt_onReadyRead(); }

private:
    virtual void virt_onDisconnected() = 0;
    virtual void virt_onReadyRead() = 0;
};

Proxy::UserSocketOutputs::~UserSocketOutputs() = default;

class NODISCARD Proxy::UserSocket final : public QTcpSocket
{
private:
    Proxy::UserSocketOutputs &m_outputs;

public:
    explicit UserSocket(const qintptr socketDescriptor, QObject *parent, UserSocketOutputs &outputs)
        : QTcpSocket{parent}
        , m_outputs{outputs}
    {
        if (!setSocketDescriptor(socketDescriptor)) {
            throw std::runtime_error("failed to accept user socket");
        }
        setSocketOption(QAbstractSocket::LowDelayOption, true);
        setSocketOption(QAbstractSocket::KeepAliveOption, true);
        QObject::connect(this, &QAbstractSocket::disconnected, this, [this]() {
            m_outputs.onDisconnected();
        });
        QObject::connect(this, &QIODevice::readyRead, this, [this]() { m_outputs.onReadyRead(); });
    }
    ~UserSocket() final;

public:
    void gracefulShutdown()
    {
        flush();
        disconnectFromHost();
    }
};

Proxy::UserSocket::~UserSocket()
{
    gracefulShutdown();
}

QPointer<Proxy> Proxy::allocInit(MapData &md,
                                 Mmapper2PathMachine &pm,
                                 PrespammedPath &pp,
                                 Mmapper2Group &gm,
                                 MumeClock &mc,
                                 MapCanvas &mca,
                                 GameObserver &go,
                                 qintptr &socketDescriptor,
                                 ConnectionListener &listener)
{
    auto proxy = makeQPointer<Proxy>(
        Badge<Proxy>{}, md, pm, pp, gm, mc, mca, go, socketDescriptor, listener);
    deref(proxy).init();
    return proxy;
}

Proxy::Proxy(Badge<Proxy>,
             MapData &md,
             Mmapper2PathMachine &pm,
             PrespammedPath &pp,
             Mmapper2Group &gm,
             MumeClock &mc,
             MapCanvas &mca,
             GameObserver &go,
             qintptr &socketDescriptor,
             ConnectionListener &listener)
    : QObject(&listener)
    , m_mapData(md)
    , m_pathMachine(pm)
    , m_prespammedPath(pp)
    , m_groupManager(gm)
    , m_mumeClock(mc)
    , m_mapCanvas(mca)
    , m_gameObserver(go)
    , m_socketDescriptor(socketDescriptor)
    // REVISIT: It would be better to just pass in the MainWindow directly.
    , m_mainWindow{::getMainWindow(listener)}
{
    //
}

Proxy::~Proxy()
{
    // This can happen as a result of the user hitting Alt-F4 to close the MMapper window.
    sendNewlineToUser();
    sendStatusToUser("MMapper proxy is shutting down.");

    {
        qDebug() << "disconnecting mud socket...";
        getMudSocket().disconnectFromHost();
    }

    {
        qDebug() << "disconnecting user socket...";
        getUserSocket().gracefulShutdown();
    }

    {
        auto &remoteEdit = deref(m_remoteEdit);
        remoteEdit.onDisconnected();
        remoteEdit.disconnect(); // disconnect all signals
        remoteEdit.deleteLater();
    }

    destroyPipelineObjects();
}

void Proxy::destroyPipelineObjects()
{
    qDebug() << "disconnecting proxy";
    getPipeline().apis.sendToUserLifetime.reset();
    getPipeline().mud.mudSocket.reset();
    getPipeline().user.userSocket.reset();
    m_pipeline.reset();
}

void Proxy::allocPipelineObjects()
{
    if (m_pipeline != nullptr) {
        std::abort();
    }

    m_pipeline = std::make_unique<Pipeline>();

    // Two main paths:
    // UserSocket -> UserTelnet -> UserTelnetFilter -> (User)Parser
    // MudSocket -> MudTelnet -> MudTelnetFilter -> MpiFilter -> { RemoteEdit or (Mud)Parser }
    //
    // Technically MudTelnetFilter 100% required for MpiFilter, because its protocol
    // is based on newlines, and it's sensitive to the difference between "\n" and "\r\n",
    // but UserTelnetFilter is just a buffer for Parser.
    //
    // TODO: refactor the Parser into UserParser and MudParser.
    // The distinction is already partly in place for AbstractParser (User)
    //   vs the XmlParser (Mud) which is being converted from Xml to Gmcp.

    allocUserSocket();
    allocMudSocket();

    allocUserTelnet();
    allocMudTelnet();

    allocMpiFilter();
    allocRemoteEdit();

    allocParser();

    auto &lifetime = getPipeline().apis.sendToUserLifetime.emplace();
    global::registerSendToUser(lifetime, [this](const QString &str) {
        //
        sendToUser(SendToUserSourceEnum::FromMMapper, str);
        sendPromptToUser();
    });
}

void Proxy::allocUserSocket()
{
    // The only reason this class exist is for symmetry with the other output interfaces;
    // the Proxy::UserSocket class could just call these directly without using a virtual interface.
    struct NODISCARD LocalUserSocketOutputs final : public UserSocketOutputs
    {
    private:
        Proxy &m_proxy;

    public:
        explicit LocalUserSocketOutputs(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        NODISCARD Proxy &getProxy() { return m_proxy; }

    private:
        void virt_onDisconnected() final
        {
            qDebug() << "user socket disconnected";
            getProxy().userTerminatedConnection();
        }
        void virt_onReadyRead() final { getProxy().processUserStream(); }
    };

    auto &pipe = getPipeline();
    auto &out = pipe.outputs.user.userSocketOutputs = std::make_unique<LocalUserSocketOutputs>(
        *this);
    pipe.user.userSocket = std::make_unique<UserSocket>(m_socketDescriptor, this, deref(out));
}

void Proxy::allocMudSocket()
{
    struct NODISCARD LocalMumeSocketOutputs final : public MumeSocketOutputs
    {
    private:
        Proxy &m_proxy;

    public:
        explicit LocalMumeSocketOutputs(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        NODISCARD Proxy &getProxy() { return m_proxy; }
        NODISCARD MudTelnet &getMudTelnet() { return getProxy().getMudTelnet(); }
        NODISCARD MumeXmlParser &getMudParser() { return getProxy().getMudParser(); }
        NODISCARD RemoteEdit &getRemoteEdit() { return getProxy().getRemoteEdit(); }
        NODISCARD UserTelnet &getUserTelnet() { return getProxy().getUserTelnet(); }
        NODISCARD Mmapper2Group &getGroupManager() { return getProxy().getGroupManager(); }
        NODISCARD MainWindow &getMainWindow() { return getProxy().getMainWindow(); }
        NODISCARD GameObserver &getGameObserver() { return getProxy().getGameObserver(); }

    private:
        void virt_onConnected() final
        {
            qDebug() << "mud socket connected";
            // It's a historical accident that GameObserver is first. It should probably be last.
            getGameObserver().observeConnected();
            getUserTelnet().onConnected();
            getProxy().onMudConnected();
        }

        void virt_onSocketWarning(const AnsiWarningMessage &warning) final
        {
            getProxy().sendWarningToUser(warning);
        }

        void virt_onSocketError(const QString &msg) final
        {
            getMudParser().onReset();
            getGroupManager().onReset();
            getProxy().onMudError(msg);
        }

        void virt_onSocketStatus(const QString &msg) final
        {
            getProxy().sendStatusToUser(msg.toUtf8().toStdString());
        }

        void virt_onDisconnected() final
        {
            qDebug() << "mud socket disconnected";
            getMudTelnet().onDisconnected();
            getMudParser().onReset();
            getGroupManager().onReset();
            getProxy().mudTerminatedConnection();
            getRemoteEdit().onDisconnected();
        }

        void virt_onProcessMudStream(const TelnetIacBytes &bytes) final
        {
            getMudTelnet().onAnalyzeMudStream(bytes);
        }

        void virt_onLog(const QString &msg) final
        {
            // Historically this has said "Proxy", even though it's from the MudSocket;
            // if we're going to do that, then we might as well just call getProxy().log().
            if ((false)) {
                getProxy().log(msg);
            } else {
                getMainWindow().slot_log("Proxy", msg);
            }
        }
    };

    auto &pipe = getPipeline();
    auto &out = pipe.outputs.mud.mudSocketOutputs = std::make_unique<LocalMumeSocketOutputs>(*this);
    auto &outputs = deref(out);
    pipe.mud.mudSocket = std::make_unique<MumeFallbackSocket>(this, outputs);
}

void Proxy::allocUserTelnet()
{
    struct NODISCARD LocalUserTelnetOutputs final : public UserTelnetOutputs
    {
    private:
        Proxy &m_proxy;

    public:
        explicit LocalUserTelnetOutputs(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        NODISCARD Proxy &getProxy() { return m_proxy; }
        NODISCARD bool hasConnectedUserSocket() { return getProxy().hasConnectedUserSocket(); }
        NODISCARD MudTelnet &getMudTelnet() { return getProxy().getMudTelnet(); }
        NODISCARD TelnetLineFilter &getUserTelnetFilter()
        {
            return getProxy().getUserTelnetFilter();
        }
        NODISCARD UserSocket &getUserSocket() { return getProxy().getUserSocket(); }

    private:
        void virt_onAnalyzeUserStream(const RawBytes &rawBytes, const bool goAhead) final
        {
            // inbound (from user)
            getUserTelnetFilter().receive(rawBytes, goAhead);
        }

        void virt_onSendToSocket(const TelnetIacBytes &bytes) final
        {
            if (!hasConnectedUserSocket()) {
                qWarning() << "tried to send bytes to closed user socket";
                return;
            }
            // outbound (to user)
            getUserSocket().write(bytes.getQByteArray());
        }
        void virt_onRelayGmcpFromUserToMud(const GmcpMessage &gmcp) final
        {
            // forwarded (to mud)
            getMudTelnet().onGmcpToMud(gmcp);
        }

        void virt_onRelayNawsFromUserToMud(int w, int h) final
        {
            // forwarded (to mud)
            getMudTelnet().onRelayNaws(w, h);
        }
        void virt_onRelayTermTypeFromUserToMud(const TelnetTermTypeBytes &bytes) final
        {
            // forwarded (to mud)
            getMudTelnet().onRelayTermType(bytes);
        }
    };

    auto &pipe = getPipeline();
    auto &out = pipe.outputs.user.userTelnetOutputs = std::make_unique<LocalUserTelnetOutputs>(
        *this);
    pipe.user.userTelnet = std::make_unique<UserTelnet>(deref(out));

    // Telnet -> LineFilter -> Parser
    //
    // note: backspaces are not processed here, because the line filter is really just a buffer
    // so entire user commands are sent at once to the parser. Handling backspaces requires
    // knowledge of the position within the line, so they're processed in the parser.
    pipe.user.userTelnetFilter
        = std::make_unique<TelnetLineFilter>(TelnetLineFilter::OptionBackspacesEnum::No,
                                             [this](const TelnetData &data) {
                                                 getUserParser().slot_parseNewUserInput(data);
                                             });
}

void Proxy::allocMudTelnet()
{
    struct NODISCARD LocalMudTelnetOutputs final : public MudTelnetOutputs
    {
    private:
        Proxy &m_proxy;

    public:
        explicit LocalMudTelnetOutputs(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        NODISCARD Proxy &getProxy() { return m_proxy; }
        NODISCARD TelnetLineFilter &getMudTelnetFilter() { return getProxy().getMudTelnetFilter(); }
        NODISCARD UserTelnet &getUserTelnet() { return getProxy().getUserTelnet(); }
        NODISCARD MumeXmlParser &getMudParser() { return getProxy().getMudParser(); }
        NODISCARD Mmapper2Group &getGroupManager() { return getProxy().getGroupManager(); }
        NODISCARD MumeClock &getMumeClock() { return getProxy().getMumeClock(); }
        NODISCARD GameObserver &getGameObserver() { return getProxy().getGameObserver(); }

    private:
        void virt_onAnalyzeMudStream(const RawBytes &bytes, bool goAhead) final
        {
            // inbound (from mud)
            getMudTelnetFilter().receive(bytes, goAhead);
        }

        void virt_onSendToSocket(const TelnetIacBytes &bytes) final
        {
            // outbound (to mud)
            getProxy().onSendToMudSocket(bytes);
        }

        void virt_onRelayEchoMode(const bool echo) final
        {
            // forwarded (to user)
            getUserTelnet().onRelayEchoMode(echo);

            // observers
            getGameObserver().observeToggledEchoMode(echo);
        }

        void virt_onRelayGmcpFromMudToUser(const GmcpMessage &msg) final
        {
            if (msg.isMumeClientView() || msg.isMumeClientEdit() || msg.isMumeClientCancelEdit()
                || msg.isMumeClientError() || msg.isMumeClientWrite() || msg.isMumeClientXml()) {
                // this is a private message between MUME and mmapper.
                qWarning() << "MUME.Client message was almost sent to the user.";
                return;
            }

            // forwarded (to user)
            getUserTelnet().onGmcpToUser(msg);

            // REVISIT: should parser be first?
            getGroupManager().slot_parseGmcpInput(msg);
            getMudParser().slot_parseGmcpInput(msg);
            getGameObserver().observeSentToUserGmcp(msg);
        }

        void virt_onSendGameTimeToClock(const MsspTime &msspTime) final
        {
            // special parsing of game time (from mud)
            getMumeClock().parseMSSP(msspTime);
        }

        void virt_onSendMSSPToUser(const TelnetMsspBytes &bytes) final
        {
            // forwarded (to user)
            getUserTelnet().onSendMSSPToUser(bytes);
        }

        void virt_onTryCharLogin() final
        {
            const auto &account = getConfig().account;
            if (account.rememberLogin && !account.accountName.isEmpty() && account.accountPassword) {
                // fetch asynchronously from keychain
                getProxy().getPasswordConfig().getPassword();
            }
        }

        void virt_onMumeClientView(const QString &title, const QString &body) final
        {
            getProxy().getMpiFilterFromMud().receiveMpiView(title, body);
        }
        void virt_onMumeClientEdit(const RemoteSessionId id,
                                   const QString &title,
                                   const QString &body) final
        {
            getProxy().getMpiFilterFromMud().receiveMpiEdit(id, title, body);
        }
        void virt_onMumeClientError(const QString &errmsg) final
        {
            qInfo() << errmsg;
            getProxy().sendToUser(SendToUserSourceEnum::FromMMapper,
                                  QString("MUME.Client protocol error: %1").arg(errmsg));
        }
    };

    auto &pipe = getPipeline();
    auto &out = pipe.outputs.mud.mudTelnetOutputs = std::make_unique<LocalMudTelnetOutputs>(*this);
    pipe.mud.mudTelnet = std::make_unique<MudTelnet>(deref(out));

    // Telnet -> LineFilter -> Parser
    //
    // note: backspaces are processed here for "twiddlers" displayed as 1-letter prompts
    // overwritten by backspace to simulate a rotating bar.
    pipe.mud.mudTelnetFilter
        = std::make_unique<TelnetLineFilter>(TelnetLineFilter::OptionBackspacesEnum::Yes,
                                             [this](const TelnetData &data) {
                                                 getMudParser().slot_parseNewMudInput(data);
                                             });
}

void Proxy::allocParser()
{
    struct NODISCARD LocalParserOutputs final : public AbstractParserOutputs
    {
    private:
        Proxy &m_proxy;
        QString m_lastPrompt;
        bool m_wasCompact = false;
        bool m_wasPrompt = false;
        size_t m_newlines = 0;

    public:
        explicit LocalParserOutputs(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        NODISCARD Proxy &getProxy() { return m_proxy; }
        NODISCARD MudTelnet &getMudTelnet() { return getProxy().getMudTelnet(); }
        NODISCARD UserTelnet &getUserTelnet() { return getProxy().getUserTelnet(); }
        NODISCARD GameObserver &getGameObserver() { return getProxy().getGameObserver(); }
        NODISCARD MainWindow &getMainWindow() { return getProxy().getMainWindow(); }
        NODISCARD MapCanvas &getMapCanvas() { return getProxy().getMapCanvas(); }
        NODISCARD Mmapper2PathMachine &getPathMachine() { return getProxy().getPathMachine(); }
        NODISCARD PrespammedPath &getPrespam() { return getProxy().getPrespam(); }

    private:
        void virt_onSendToMud(const QString &s) final
        {
            m_wasPrompt = false;
            getMudTelnet().onSendToMud(s);

            if (getMudTelnet().getEchoMode()) {
                getGameObserver().observeSentToMud(s);
            }
        }

        // FIXME: This function is way too complicated, and the special newline/prompt handling
        // may also need to be done at a different point in the pipeline.
        void virt_onSendToUser(const SendToUserSourceEnum source,
                               const QString &s,
                               const bool goAhead) final
        {
            bool isTwiddler = false;
            bool isPrompt = false;

            switch (source) {
            case SendToUserSourceEnum::NoLongerPrompted:
                assert(s.isEmpty());
                m_wasPrompt = false;
                return;
            case SendToUserSourceEnum::DuplicatePrompt:
            case SendToUserSourceEnum::SimulatedPrompt:
                isPrompt = true;
                break;
            case SendToUserSourceEnum::FromMud:
                if (s.isEmpty()) {
                    break;
                }
                isTwiddler = s.back() == char_consts::C_BACKSPACE;
                isPrompt = s.back() != char_consts::C_NEWLINE && !isTwiddler;
                if (isPrompt) {
                    m_wasCompact = (m_newlines == 1);
                }
                break;
            case SendToUserSourceEnum::SimulatedOutput:
            case SendToUserSourceEnum::FromMMapper:
                break;
            }

            if (!goAhead && s.isEmpty()) {
                return;
            }

            if (isPrompt && m_wasPrompt && m_lastPrompt == s) {
                return;
            }

            const bool endsInNewline = s.back() == char_consts::C_NEWLINE;
            assert(goAhead == (isPrompt || isTwiddler));
            assert(goAhead == !endsInNewline);

            auto startsWithNewline = [](QStringView sv) {
                if (sv.isEmpty()) {
                    return false;
                }
                if (sv.front() == char_consts::C_CARRIAGE_RETURN) {
                    sv = sv.mid(1);
                }
                return !sv.empty() && sv.front() == char_consts::C_NEWLINE;
            };

            // The logic for isMissingNewline may be incomplete; expect more bugs here.
            const bool isMissingNewline = m_wasPrompt ? (!isTwiddler && !startsWithNewline(s))
                                                      : (isPrompt
                                                         && (m_wasCompact ? m_newlines == 0
                                                                          : m_newlines < 2));
            if (isMissingNewline) {
                // add the missing newline.
                getUserTelnet().onSendToUser(string_consts::S_NEWLINE, false);
            }
            getUserTelnet().onSendToUser(s, goAhead);

            // FIXME: This is probably in the wrong location; the game observer should only
            // receive messages originating from the Mud; however in this location it also
            // receives some (but possibly not all) messages originating from MMapper's
            // command parser.
            getGameObserver().observeSentToUser(s);

            switch (source) {
            case SendToUserSourceEnum::FromMud:
            case SendToUserSourceEnum::DuplicatePrompt:
            case SendToUserSourceEnum::SimulatedPrompt:
            case SendToUserSourceEnum::SimulatedOutput:
            case SendToUserSourceEnum::FromMMapper:
            case SendToUserSourceEnum::NoLongerPrompted:
                break;
            }

            m_wasPrompt = isPrompt || isTwiddler;
            if (m_wasPrompt) {
                m_lastPrompt = s;
            } else {
                mmqt::foreachLine(s, [this](QStringView sv, bool hasNewline) {
                    if (!hasNewline) {
                        m_newlines = 0;
                        return;
                    }
                    if (!sv.empty() && sv.back() == char_consts::C_CARRIAGE_RETURN) {
                        sv.chop(1);
                    }
                    if (sv.isEmpty()) {
                        ++m_newlines;
                    } else {
                        m_newlines = 1;
                    }
                });
            }
        }

        void virt_onHandleParseEvent(const SigParseEvent &sigParseEvent) final
        {
            getPathMachine().slot_handleParseEvent(sigParseEvent);
        }

        void virt_onReleaseAllPaths() final { getPathMachine().slot_releaseAllPaths(); }
        void virt_onShowPath(const CommandQueue &path) final { getPrespam().slot_setPath(path); }
        void virt_onMapChanged() final { getMapCanvas().slot_mapChanged(); }
        void virt_onGraphicsSettingsChanged() final { getMapCanvas().graphicsSettingsChanged(); }
        void virt_onLog(const QString &mod, const QString &msg) final
        {
            getMainWindow().slot_log(mod, msg);
        }
        void virt_onNewRoomSelection(const SigRoomSelection &sel) final
        {
            getMapCanvas().slot_setRoomSelection(sel);
        }

        // (via user command)
        void virt_onSetMode(const MapModeEnum mode) final { getMainWindow().slot_setMode(mode); }
    };

    auto &pipe = getPipeline();
    auto &conn = pipe.apis.proxyMudConnection = std::make_unique<ProxyMudConnectionApi>(*this);
    auto &gmcp = pipe.apis.proxyGmcp = std::make_unique<ProxyUserGmcpApi>(*this);
    auto &out = pipe.outputs.parserXmlOutputs = std::make_unique<LocalParserOutputs>(*this);

    // REVISIIT: does CTimers actually need a parent?
    // if so, figure out what and allocate it into the pipeline if necessary?
    QObject *fakeCTimersParent = nullptr;

    auto &parserCommon = pipe.common.parserCommonData;
    parserCommon = std::make_unique<ParserCommonData>(fakeCTimersParent);

    /* this duplication is ridiculous, but it's painful to tease these two apart
     * because compiling takes so long. */
    pipe.mud.mudParser = std::make_unique<MumeXmlParser>(m_mapData,
                                                         m_mumeClock,
                                                         deref(conn),
                                                         deref(gmcp),
                                                         m_groupManager.getGroupManagerApi(),
                                                         this,
                                                         deref(out),
                                                         deref(parserCommon));
    pipe.user.userParser = std::make_unique<AbstractParser>(m_mapData,
                                                            m_mumeClock,
                                                            deref(conn),
                                                            deref(gmcp),
                                                            m_groupManager.getGroupManagerApi(),
                                                            this,
                                                            deref(out),
                                                            deref(parserCommon));

    /* The login credentials are fetched asynchronously because the OS will prompt the user for permission */
    pipe.mud.passwordConfig = std::make_unique<PasswordConfig>(this);
    QObject::connect(pipe.mud.passwordConfig.get(),
                     &PasswordConfig::sig_error,
                     this,
                     [](const QString &err) { qWarning() << err; });

    QObject::connect(pipe.mud.passwordConfig.get(),
                     &PasswordConfig::sig_incomingPassword,
                     this,
                     [this](const QString &password) {
                         getMudTelnet().onLoginCredentials(getConfig().account.accountName,
                                                           password);
                     });
}

void Proxy::allocMpiFilter()
{
    struct NODISCARD LocalMpiFilterOutputs final : public MpiFilterOutputs
    {
    private:
        Proxy &m_proxy;

    public:
        explicit LocalMpiFilterOutputs(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        NODISCARD Proxy &getProxy() { return m_proxy; }
        NODISCARD MumeXmlParser &getMudParser() { return getProxy().getMudParser(); }
        NODISCARD RemoteEdit &getRemoteEdit() { return getProxy().getRemoteEdit(); }

    private:
        void notifyUser(const std::string_view article,
                        const std::string_view what,
                        const QString &title)
        {
            const auto color = whiteOnCyan;
            auto aos = getProxy().getSendToUserAnsiOstream();
            if (g_prefixMessagesToUser) {
                aos.writeWithColor(color.withBold(), "Info");
                aos.writeWithColor(color, ": ");
            }
            aos.writeWithColor(color, "MMapper is opening ");
            aos.writeWithColor(color, article);
            aos.writeWithColor(color, " ");
            aos.writeWithColor(color.withBold(), what);
            aos.writeWithColor(color, " window with title \"");
            aos.writeWithColor(color.withBold(), mmqt::toStdStringUtf8(title));
            aos.writeWithColor(color, "\"");
            aos.write("\n");
        }

    private:
        void virt_onEditMessage(const RemoteSessionId id,
                                const QString &title,
                                const QString &body) final
        {
            notifyUser("an", "Editor", title);
            getRemoteEdit().slot_remoteEdit(id, title, body);
        }
        void virt_onViewMessage(const QString &title, const QString &body) final
        {
            notifyUser("a", "Viewer", title);
            getRemoteEdit().slot_remoteView(title, body);
        }
        void virt_onParseNewMudInput(const TelnetData &data) final
        {
            getMudParser().slot_parseNewMudInput(data);
        }
    };

    auto &pipe = getPipeline();
    auto &out = pipe.outputs.mud.mpiFilterOutputs = std::make_unique<LocalMpiFilterOutputs>(*this);
    pipe.mud.mpiFilterFromMud = std::make_unique<MpiFilter>(deref(out));
}

void Proxy::allocRemoteEdit()
{
    // Caution: RemoteEdit outlives the proxy, since it manages windows.
    m_remoteEdit = mmqt::makeQPointer<RemoteEdit>(&m_mainWindow);

    struct NODISCARD LocalMpiFilterToMud final : public MpiFilterToMud
    {
    private:
        Proxy &m_proxy;

    public:
        explicit LocalMpiFilterToMud(Proxy &proxy)
            : m_proxy{proxy}
        {}

    private:
        void virt_submitGmcp(const GmcpMessage &gmcpMessage) final
        {
            m_proxy.getMudTelnet().onSubmitGmcpMumeClient(gmcpMessage);
        }
    };

    auto &pipe = getPipeline();
    pipe.mud.mpiFilterToMud = std::make_unique<LocalMpiFilterToMud>(*this);

    auto &remoteEdit = deref(m_remoteEdit);
    QObject::connect(&remoteEdit,
                     &RemoteEdit::sig_remoteEditCancel,
                     this,
                     [this](const RemoteSessionId id) { getMpiFilterToMud().cancelRemoteEdit(id); });

    QObject::connect(&remoteEdit,
                     &RemoteEdit::sig_remoteEditSave,
                     this,
                     [this](const RemoteSessionId id, const Latin1Bytes &content) {
                         getMpiFilterToMud().saveRemoteEdit(id, content);
                     });
}

void Proxy::init()
{
    auto initMisc = [this]() {
        // TODO: convert these from Qt signals
        QObject::connect(&m_mapData, &MapData::sig_onForcedPositionChange, this, [this]() {
            getMudParser().onForcedPositionChange();
        });
    };

    allocPipelineObjects();

    initMisc();

    log("Connection to client established ...");
    sendWelcomeToUser();
    sendSyntaxHintToUser("Type", "help", "for help.");

    connectToMud();
}

void Proxy::gmcpToMud(const GmcpMessage &msg)
{
    getMudTelnet().onGmcpToMud(msg);
}
void Proxy::gmcpToUser(const GmcpMessage &msg)
{
    getUserTelnet().onGmcpToUser(msg);
}

void Proxy::sendToMud(const QString &s)
{
    // REVISIT: this bypasses game observer, but it also appears to be unused.
    getMudTelnet().onSendToMud(s);
}

void Proxy::sendToUser(const SendToUserSourceEnum source, const QString &s)
{
    // FIXME: this is layered incorrectly
    getUserParser().sendToUser(source, s);
}

void Proxy::onMudConnected()
{
    m_serverState = ServerStateEnum::Connected;

    log("Connection to server established ...");

    // Reset clock precision to its lowest level
    m_mumeClock.setPrecision(MumeClockPrecisionEnum::UNSET);
}

void Proxy::onMudError(const QString &errorStr)
{
    m_serverState = ServerStateEnum::Error;

    qWarning() << "Mud socket error" << errorStr;
    log(errorStr);

    sendNewlineToUser();
    sendErrorToUser(mmqt::toStdStringUtf8(errorStr));

    if (!getConfig().connection.proxyConnectionStatus) {
        sendNewlineToUser();
        sendSyntaxHintToUser("You can type", "connect", "to reconnect again.");
        sendPromptToUser();
        m_serverState = ServerStateEnum::Offline;
    } else if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
        sendNewlineToUser();
        sendStatusToUser("You are now exploring the map offline.");
        sendPromptToUser();
        m_serverState = ServerStateEnum::Offline;
    } else {
        // Terminate connection
        deleteLater();
    }
}

void Proxy::userTerminatedConnection()
{
    log("User terminated connection ...");
    getMudParser().onReset();
    deleteLater();
}

void Proxy::mudTerminatedConnection()
{
    if (!isConnected()) {
        return;
    }

    m_serverState = ServerStateEnum::Disconnected;

    getUserTelnet().onRelayEchoMode(true);

    log("Mud terminated connection ...");

    sendNewlineToUser();
    sendStatusToUser("MUME closed the connection.");

    if (!getConfig().connection.proxyConnectionStatus) {
        sendNewlineToUser();
        sendSyntaxHintToUser("You can type", "connect", "to reconnect again.");
        sendPromptToUser();
    } else if (getConfig().general.mapMode == MapModeEnum::OFFLINE) {
        sendNewlineToUser();
        sendStatusToUser("You are now exploring the map offline.");
        sendPromptToUser();
    } else {
        // Terminate connection
        deleteLater();
    }
}

void Proxy::processUserStream()
{
    // REVISIT: is this "supposed" to happen? If not, just allow deref() to cause an exception.
    if (!hasConnectedUserSocket()) {
        return;
    }

    // REVISIT: check return value?
    std::ignore = io::readAllAvailable(getUserSocket(),
                                       m_buffer,
                                       [this](const QByteArray &byteArray) {
                                           assert(!byteArray.isEmpty());
                                           getUserTelnet().onAnalyzeUserStream(
                                               TelnetIacBytes{byteArray});
                                       });
}

void Proxy::onSendToMudSocket(const TelnetIacBytes &bytes)
{
    auto &mudSocket = getMudSocket();
    if (!mudSocket.isConnectedOrConnecting()) {
        sendStatusToUser("MMapper is not connected to MUME.");
        sendSyntaxHintToUser("You can type", "connect", "to play.");
        sendPromptToUser();
        return;
    }

    mudSocket.sendToMud(bytes);
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
            sendToUser(SendToUserSourceEnum::SimulatedOutput,
                       "\n\n"
                       "Welcome to the land of Middle-earth. May your visit here be... interesting.\n"
                       "Never forget! Try to role-play...\n");
            getUserParser().doMove(CommandEnum::LOOK);
            m_serverState = ServerStateEnum::Offline;
            break;
        }

        sendStatusToUser("Connecting...");
        m_serverState = ServerStateEnum::Connecting;
        getMudSocket().connectToHost();
        break;
    }
    }
}

void Proxy::disconnectFromMud()
{
    getUserTelnet().onRelayEchoMode(true);

    switch (m_serverState) {
    case ServerStateEnum::Connecting:
        // REVISIT: Can't we force it to abandon a connection attempt?
        // (The user may not want to wait for the timeout.)
        sendErrorToUser("You're still connecting.");
        break;

    case ServerStateEnum::Offline:
        m_serverState = ServerStateEnum::Initialized;
        sendStatusToUser("You disconnect your simulated link.");
        break;

    case ServerStateEnum::Connected: {
        sendStatusToUser("Disconnecting...");
        m_serverState = ServerStateEnum::Disconnecting;
        getMudSocket().disconnectFromHost();
        sendStatusToUser("Disconnected.");
        m_serverState = ServerStateEnum::Disconnected;
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

bool Proxy::isMudGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const
{
    return getMudTelnet().isGmcpModuleEnabled(mod);
}

bool Proxy::isUserGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const
{
    return getUserTelnet().isGmcpModuleEnabled(mod);
}

auto Proxy::getSendToUserAnsiOstream() -> AnsiHelper
{
    return AnsiHelper{m_lifetime, [this](const std::string &s) -> void {
                          sendToUser(SendToUserSourceEnum::FromMMapper, mmqt::toQStringUtf8(s));
                      }};
}

void Proxy::sendWelcomeToUser()
{
    const auto color = whiteOnCyan;
    auto aos = getSendToUserAnsiOstream();
    aos.writeWithColor(color.withBold(), "Welcome to MMapper!");
    if (g_showVersionInWelcomeMessage) {
        aos.writeWithColor(color, " (version ");
        aos.writeWithColor(color, getMMapperVersion());
        aos.writeWithColor(color, ")");
    }
    aos.write("\n");
}

void Proxy::sendWarningToUser(const AnsiWarningMessage &warning)
{
    const auto color = getRawAnsi(warning.fg, warning.bg);
    auto aos = getSendToUserAnsiOstream();
    aos.writeWithColor(color.withBold(), mmqt::toStdStringUtf8(warning.title));
    aos.writeWithColor(color, ": ");
    aos.writeWithColor(color, mmqt::toStdStringUtf8(warning.msg));
    aos.write("\n");
}

void Proxy::sendErrorToUser(const std::string_view msg)
{
    // REVISIT: historically the error message sent by the socket was white on cyan,
    // but the encryption warnings were white on red,
    // so these errors should probably also white on red?
    const volatile bool useRed = true;
    const auto color = useRed ? getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::red)
                              : whiteOnCyan;

    auto aos = getSendToUserAnsiOstream();
    if (g_prefixMessagesToUser) {
        aos.writeWithColor(color.withBold(), "Error");
        aos.writeWithColor(color, ": ");
    }
    aos.writeWithColor(color, msg);
    aos.write("\n");
}

void Proxy::sendStatusToUser(const std::string_view msg)
{
    const auto color = whiteOnCyan;
    auto aos = getSendToUserAnsiOstream();
    if (g_prefixMessagesToUser) {
        aos.writeWithColor(color.withBold(), "Status");
        aos.writeWithColor(color, ": ");
    }
    aos.writeWithColor(color, msg);
    aos.write("\n");
}

void Proxy::sendSyntaxHintToUser(const std::string_view before,
                                 const std::string_view cmd,
                                 const std::string_view after)
{
    const auto color = whiteOnCyan;
    const auto bold = color.withBold();
    auto aos = getSendToUserAnsiOstream();
    if (g_prefixMessagesToUser) {
        aos.writeWithColor(bold, "Hint");
        aos.writeWithColor(color, ": ");
    }
    aos.writeWithColor(color, before);
    aos.writeWithColor(color, " ");
    aos.writeWithColor(bold, getConfig().parser.prefixChar);
    aos.writeWithColor(bold, cmd);
    aos.writeWithColor(color, " ");
    aos.writeWithColor(color, after);
    aos.write("\n");
}

void Proxy::sendNewlineToUser()
{
    // TODO: find a way to avoid sending extra newlines when we assume a prompt exists,
    // and also find a way to re-send the prompt if we overwrite it.
    sendToUser(SendToUserSourceEnum::FromMMapper, "\n");
}

void Proxy::sendPromptToUser()
{
    // REVISIT: which one should this go to?
    getUserParser().sendPromptToUser();
}

void Proxy::log(const QString &msg)
{
    getMainWindow().slot_log("Proxy", msg);
}

RemoteEdit &Proxy::getRemoteEdit()
{
    return deref(m_remoteEdit);
}

bool Proxy::hasConnectedUserSocket() const
{
    // REVISIT: Is this ever actually null, or is it just disconnected?
    const auto &us = getPipeline().user.userSocket;
    return us != nullptr /* && us->state() == QAbstractSocket::ConnectedState */;
}
