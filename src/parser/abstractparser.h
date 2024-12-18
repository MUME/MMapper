#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../configuration/configuration.h"
#include "../global/StringView.h"
#include "../global/TextUtils.h"
#include "../map/CommandId.h"
#include "../map/ConnectedRoomFlags.h"
#include "../map/DoorFlags.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitsFlags.h"
#include "../map/PromptFlags.h"
#include "../map/RoomFieldVariant.h"
#include "../map/RoomHandle.h"
#include "../map/mmapper2room.h"
#include "../map/parseevent.h"
#include "../mapdata/roomselection.h"
#include "../pandoragroup/GroupManagerApi.h"
#include "../pandoragroup/mmapper2character.h"
#include "../proxy/ProxyParserApi.h"
#include "../proxy/telnetfilter.h"
#include "../timers/CTimers.h"
#include "AbstractParser-Utils.h"
#include "Action.h"
#include "CommandQueue.h"
#include "DoorAction.h"
#include "SendToUserSource.h"

#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <QArgument>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVariant>

class Coordinate;
class MapData;
class MumeClock;
class RoomFieldVariant;
class RoomFilter;
class CTimers;

namespace syntax {
class Sublist;
}

struct NODISCARD AbstractParserOutputs
{
public:
    virtual ~AbstractParserOutputs();

public:
    // sent to MudTelnet
    void onSendToMud(const QString &msg) { virt_onSendToMud(msg); }
    // sent to UserTelnet
    void onSendToUser(const SendToUserSource source, const QString &msg, const bool goAhead)
    {
        virt_onSendToUser(source, msg, goAhead);
    }
    // sent to MapCanvas
    void onMapChanged() { virt_onMapChanged(); }
    // sent to MapCanvas
    void onGraphicsSettingsChanged() { virt_onGraphicsSettingsChanged(); }
    // sent to PathMachine
    void onReleaseAllPaths() { virt_onReleaseAllPaths(); }
    // sent to MainWindow's log
    void onLog(const QString &mod, const QString &msg) { virt_onLog(mod, msg); }
    // sent to PathMachine
    // for main move/search algorithm
    // REVISIT: can we remove the signal wrapper?
    void onHandleParseEvent(const SigParseEvent &parseEvent)
    {
        virt_onHandleParseEvent(parseEvent);
    }
    // for map
    void onShowPath(const CommandQueue &path) { virt_onShowPath(path); }
    // sent to MapCanvas
    // REVISIT: can we remove the signal wrapper?
    void onNewRoomSelection(const SigRoomSelection &roomSelection)
    {
        virt_onNewRoomSelection(roomSelection);
    }
    // sent to MainWindow
    // for commands that set the mode (emulation, play, map)
    // these are connected to MainWindow
    void onSetMode(const MapModeEnum mode) { virt_onSetMode(mode); }
    // sent to MapCanvas
    void onInfomarksChanged() { virt_onInfomarksChanged(); }

private:
    // sent to MudTelnet
    virtual void virt_onSendToMud(const QString &) = 0;
    // sent to UserTelnet
    virtual void virt_onSendToUser(SendToUserSource source, const QString &, bool goAhead) = 0;
    // sent to MapCanvas
    virtual void virt_onMapChanged() = 0;
    // sent to MapCanvas
    virtual void virt_onGraphicsSettingsChanged() = 0;
    // sent to PathMachine
    virtual void virt_onReleaseAllPaths() = 0;
    // sent to MainWindow's log
    virtual void virt_onLog(const QString &mod, const QString &msg) = 0;
    // sent to PathMachine
    // for main move/search algorithm
    // REVISIT: can we remove the signal wrapper?
    virtual void virt_onHandleParseEvent(const SigParseEvent &parseEvent) = 0;
    // for map
    virtual void virt_onShowPath(const CommandQueue &) = 0;
    // sent to MapCanvas
    // REVISIT: can we remove the signal wrapper?
    virtual void virt_onNewRoomSelection(const SigRoomSelection &roomSelection) = 0;
    // sent to MainWindow
    // for commands that set the mode (emulation, play, map)
    // these are connected to MainWindow
    virtual void virt_onSetMode(MapModeEnum) = 0;
    // sent to MapCanvas
    virtual void virt_onInfomarksChanged() = 0;
};

struct NODISCARD ParserCommonData final
{
public:
    QString exits; // nullString
    ExitsFlagsType exitsFlags;
    PromptFlagsType promptFlags;
    ConnectedRoomFlagsType connectedRoomFlags;
    RoomTerrainEnum terrain;

public:
    QString lastPrompt;
    CommandQueue queue;
    bool overrideSendPrompt = false;
    bool trollExitMapping = false;

public:
    // accessed by initActionMap
    CTimers timers;

public:
    explicit ParserCommonData(QObject *parent)
        : timers{parent}
    {}
};

class NODISCARD_QOBJECT ParserCommon : public QObject
{
    Q_OBJECT

protected:
    MumeClock &m_mumeClock;
    MapData &m_mapData;

protected:
    GroupManagerApi &m_group;
    ProxyUserGmcpApi &m_proxyUserGmcp;
    AbstractParserOutputs &m_outputs;

protected:
    ParserCommonData &m_commonData;

protected:
    explicit ParserCommon(QObject *const parent,
                          MumeClock &mumeClock,
                          MapData &mapData,
                          GroupManagerApi &group,
                          ProxyUserGmcpApi &proxyUserGmcp,
                          AbstractParserOutputs &outputs,
                          ParserCommonData &commonData)
        : QObject{parent}
        , m_mumeClock{mumeClock}
        , m_mapData{mapData}
        , m_group{group}
        , m_proxyUserGmcp{proxyUserGmcp}
        , m_outputs{outputs}
        , m_commonData{commonData}
    {}

protected:
    NODISCARD CommandQueue &getQueue() { return m_commonData.queue; }
    NODISCARD CTimers &getTimers() { return m_commonData.timers; }

protected:
    void log(const QString &a, const QString &b) { m_outputs.onLog(a, b); }

public:
    void sendToUser(const SendToUserSource source, const QString &s, const bool goAhead)
    {
        m_outputs.onSendToUser(source, s, goAhead);
    }

    inline void sendToUser(const SendToUserSource source, const QByteArray &arr)
    {
        sendToUser(source, QString::fromUtf8(arr), false);
    }
    inline void sendToUser(const SendToUserSource source, const std::string_view s)
    {
        sendToUser(source, mmqt::toQStringUtf8(s));
    }
    inline void sendToUser(const SendToUserSource source, const char *const s)
    {
        assert(s != nullptr);
        if (s != nullptr) {
            sendToUser(source, std::string_view{s});
        }
    }
    inline void sendToUser(const SendToUserSource source, const QString &s)
    {
        sendToUser(source, s, false);
    }

protected:
    void onNewRoomSelection(const SigRoomSelection &roomSelection)
    {
        m_outputs.onNewRoomSelection(roomSelection);
    }

public:
    void sendPromptToUser();

public:
    void sendPromptToUser(const RoomHandle &r);
    void sendPromptToUser(char light, char terrain);
    void sendPromptToUser(RoomLightEnum lightType, RoomTerrainEnum terrainType);

protected:
    void onHandleParseEvent(const SigParseEvent &parseEvent)
    {
        m_outputs.onHandleParseEvent(parseEvent);
    }

protected:
    void emulateExits(AnsiOstream &, CommandEnum move);
    void sendRoomExitsInfoToUser(AnsiOstream &, const RoomHandle &r);
    void sendRoomExitsInfoToUser(const RoomHandle &r);

protected:
    void setConnectedRoomFlag(DirectSunlightEnum light, ExitDirEnum dir);
    void setExitFlags(ExitFlags flag, ExitDirEnum dir);

protected:
    NODISCARD RoomId getNextPosition() const;
    NODISCARD RoomId getTailPosition() const;

protected:
    void pathChanged() { m_outputs.onShowPath(m_commonData.queue); }
    void onReleaseAllPaths() { m_outputs.onReleaseAllPaths(); }

protected:
    // accesses m_queue
    void clearQueue();
};

class MumeXmlParserBase : public ParserCommon
{
private:
    ActionRecordMap m_actionMap;

protected:
    explicit MumeXmlParserBase(QObject *const parent,
                               MumeClock &mumeClock,
                               MapData &mapData,
                               GroupManagerApi &group,
                               ProxyUserGmcpApi &proxyUserGmcp,
                               AbstractParserOutputs &outputs,
                               ParserCommonData &parserCommonData)
        : ParserCommon{parent, mumeClock, mapData, group, proxyUserGmcp, outputs, parserCommonData}
    {
        initActionMap();
    }
    ~MumeXmlParserBase() override;

private:
    void initActionMap();

protected:
    void parseExits(std::ostream &);
    NODISCARD bool evalActionMap(StringView line);

public:
    // accesses m_lastPrompt and m_queue,
    // but it's also only called from the proxy.
    void onReset();
    void onForcedPositionChange();
};

// TODO: rename this to UserInputParser
class NODISCARD_QOBJECT AbstractParser final : public ParserCommon
{
    Q_OBJECT

private:
    class ParseRoomHelper;

private:
    ProxyMudConnectionApi &m_proxyMudConnection;

private:
    // NOTE: This is only shared because std::unique_ptr<> does not play nice with opaque types,
    // and the definition of ParseRoomHelper is not visible to the ctor. Nothing besides
    // parseRoom() should ever have any reason to use this pointer!
    std::shared_ptr<ParseRoomHelper> m_parseRoomHelper;

public:
    using HelpCallback = std::function<void(const std::string &name)>;
    using ParserCallback
        = std::function<bool(const std::vector<StringView> &matched, StringView args)>;
    struct NODISCARD ParserRecord final
    {
        std::string fullCommand;
        ParserCallback callback;
        HelpCallback help;
    };
    using ParserRecordMap = std::map<std::string, ParserRecord>;

private:
    ParserRecordMap m_specialCommandMap;
    NODISCARD static char getPrefixChar()
    {
        static const char &g_prefixChar = getConfig().parser.prefixChar;
        return g_prefixChar;
    }

private:
    QTimer m_offlineCommandTimer;

public:
    explicit AbstractParser(MapData &,
                            MumeClock &,
                            ProxyMudConnectionApi &,
                            ProxyUserGmcpApi &,
                            GroupManagerApi &,
                            QObject *parent,
                            AbstractParserOutputs &outputs,
                            ParserCommonData &commonData);
    ~AbstractParser() override;

    void doMove(CommandEnum cmd);

protected:
    void offlineCharacterMove(CommandEnum direction);
    void offlineCharacterMove() { offlineCharacterMove(CommandEnum::UNKNOWN); }
    void sendRoomInfoToUser(const RoomHandle &);

    // command handling
    void performDoorCommand(ExitDirEnum direction, DoorActionEnum action);

public:
    NODISCARD bool parseUserCommands(const QString &command);

    void searchCommand(const RoomFilter &f);
    void dirsCommand(const RoomFilter &f);

private:
    void setMode(const MapModeEnum mode) { m_outputs.onSetMode(mode); }
    void parseSpecialCommand(StringView);
    NODISCARD bool parseSimpleCommand(const QString &str);

    void showDoorCommandHelp();
    void showMumeTime();
    void showHelp();
    void showMiscHelp();
    void showCommandPrefix();
    void showSyntax(const char *rest);
    void showHelpCommands(bool showAbbreviations);

    void showHeader(const QString &s);

    NODISCARD ExitDirEnum tryGetDir(StringView &words);

    void parseSetCommand(StringView view);
    void parseDirections(StringView view);
    void parseSearch(StringView view);

    NODISCARD bool setCommandPrefix(char prefix);

    void openVoteURL();
    void doBackCommand();
    void doConfig(StringView view);

    NODISCARD bool isConnected();
    void doConnectToHost();
    void doDisconnectFromHost();
    void doMapCommand(StringView rest);
    void doRemoveDoorNamesCommand();
    void doMapDiff();
    void doGenerateBaseMap();
    void doSearchCommand(StringView view);
    void doGetDirectionsCommand(StringView view);

    void initSpecialCommandMap();
    void addSpecialCommand(const char *s,
                           int minLen,
                           const ParserCallback &callback,
                           const HelpCallback &help);
    NODISCARD bool evalSpecialCommandMap(StringView args);

    void parseHelp(StringView words);
    void parseMark(StringView input);
    void parseRoom(StringView input);
    void parseGroup(StringView input);
    void parseTimer(StringView input);

    NODISCARD bool parseDoorAction(DoorActionEnum dat, StringView words);

public:
    friend AbstractParser &operator<<(AbstractParser &self, const std::string_view s)
    {
        self.sendToUser(SendToUserSource::FromMMapper, s);
        return self;
    }
    inline void sendOkToUser() { send_ok(*this); }

protected:
    void mapChanged() { m_outputs.onMapChanged(); }
    void infomarksChanged() { m_outputs.onInfomarksChanged(); }

private:
    void graphicsSettingsChanged() { m_outputs.onGraphicsSettingsChanged(); }

    void sendToMud(const QByteArray &msg) = delete;
    void sendToMud(const QString &msg) { m_outputs.onSendToMud(msg); }

private:
    void eval(std::string_view name,
              const std::shared_ptr<const syntax::Sublist> &syntax,
              StringView input);

private:
    NODISCARD RoomId getCurrentRoomId() const;
    NODISCARD RoomId getOtherRoom(int otherRoomId) const;
    NODISCARD RoomId getOptionalOtherRoom(const Vector &v, size_t index) const;

    void applySingleChange(const Change &change);
    void applyChanges(const ChangeList &changes);
    void doOfflineCharacterMove();

private:
    void executeMudCommand(const QString &command);
    void timersUpdate(const std::string &text);

public slots:
    void slot_parseNewUserInput(const TelnetData &);
};

NODISCARD extern QString normalizeStringCopy(QString str);

namespace test {
extern void testAbstractParser();
} // namespace test
