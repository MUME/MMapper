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

class NODISCARD_QOBJECT AbstractParser : public QObject
{
    Q_OBJECT

private:
    class ParseRoomHelper;

protected:
    static const QString nullString;
    static const QByteArray emptyByteArray;

protected:
    MumeClock &m_mumeClock;
    MapData &m_mapData;
    CTimers &m_timers;

private:
    const ProxyParserApi m_proxy;
    const GroupManagerApi m_group;

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
    ActionRecordMap m_actionMap;

protected:
    QString m_exits = nullString;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;
    RoomTerrainEnum m_terrain;

protected:
    QString m_lastPrompt;
    CommandQueue m_queue;
    bool m_compactMode = false;
    bool m_overrideSendPrompt = false;
    bool m_trollExitMapping = false;

private:
    QTimer m_offlineCommandTimer;

public:
    explicit AbstractParser(
        MapData &, MumeClock &, ProxyParserApi, GroupManagerApi, CTimers &timers, QObject *parent);
    ~AbstractParser() override;

    void doMove(CommandEnum cmd);
    void sendPromptToUser();
    void sendScoreLineEvent(const QString &arr);
    void sendPromptLineEvent(const QString &arr);

protected:
    void offlineCharacterMove(CommandEnum direction);
    void offlineCharacterMove() { offlineCharacterMove(CommandEnum::UNKNOWN); }
    void sendRoomInfoToUser(const RoomPtr &);
    void sendPromptToUser(const RoomHandle &r);
    void sendPromptToUser(char light, char terrain);
    void sendPromptToUser(RoomLightEnum lightType, RoomTerrainEnum terrainType);

    void sendRoomExitsInfoToUser(AnsiOstream &, const RoomPtr &r);
    void sendRoomExitsInfoToUser(const RoomPtr &r);

    NODISCARD RoomId getNextPosition() const;
    NODISCARD RoomId getTailPosition() const;

    // command handling
    void performDoorCommand(ExitDirEnum direction, DoorActionEnum action);
    void genericDoorCommand(QString command, ExitDirEnum direction);

public:
    void setExitFlags(ExitFlags flag, ExitDirEnum dir);
    void setConnectedRoomFlag(DirectSunlightEnum light, ExitDirEnum dir);

    void emulateExits(AnsiOstream &, CommandEnum move);
    void parseExits(std::ostream &);
    NODISCARD bool parseUserCommands(const QString &command);
    NODISCARD static QString normalizeStringCopy(QString str);

    void searchCommand(const RoomFilter &f);
    void dirsCommand(const RoomFilter &f);

    NODISCARD bool evalActionMap(StringView line);

private:
    void setMode(MapModeEnum mode);
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

    void receiveMudServerStatus(const TelnetMsspBytes &);

    NODISCARD ExitDirEnum tryGetDir(StringView &words);

    void parseSetCommand(StringView view);
    void parseDirections(StringView view);
    void parseSearch(StringView view);

    NODISCARD bool setCommandPrefix(char prefix);

    void openVoteURL();
    void doBackCommand();
    void doConfig(StringView view);
    void doConnectToHost();
    void doDisconnectFromHost();
    void doMapCommand(StringView rest);
    void doRemoveDoorNamesCommand();
    void doMapDiff();
    void doGenerateBaseMap();
    void doSearchCommand(StringView view);
    void doGetDirectionsCommand(StringView view);

    void initActionMap();

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
    inline void sendToUser(const QByteArray &arr) { sendToUser(QString::fromUtf8(arr), false); }
    inline void sendToUser(const std::string_view s) { sendToUser(mmqt::toQStringUtf8(s)); }
    inline void sendToUser(const char *const s)
    {
        assert(s != nullptr);
        if (s != nullptr) {
            sendToUser(std::string_view{s});
        }
    }
    inline void sendToUser(const QString &s) { sendToUser(s, false); }
    friend AbstractParser &operator<<(AbstractParser &self, const std::string_view s)
    {
        self.sendToUser(s);
        return self;
    }
    inline void sendOkToUser() { send_ok(*this); }

protected:
    inline void sendToUser(const QString &s, const bool goAhead)
    {
        emit sig_sendToUser(s, goAhead);
    }
    void pathChanged() { emit sig_showPath(m_queue); }
    void mapChanged() { emit sig_mapChanged(); }

protected:
    void log(const QString &a, const QString &b) { emit sig_log(a, b); }

private:
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }

    void sendToMud(const QByteArray &msg) = delete;
    void sendToMud(const QString &msg) { emit sig_sendToMud(msg); }

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
    void clearQueue();

signals:
    // telnet
    void sig_sendToMud(const QString &);
    void sig_sendToUser(const QString &, bool goAhead);
    void sig_mapChanged();
    void sig_graphicsSettingsChanged();
    void sig_releaseAllPaths();

    // used to log
    void sig_log(const QString &, const QString &);

    // for main move/search algorithm
    void sig_handleParseEvent(const SigParseEvent &);
    void sig_setCharRoomIdFromServer(ServerRoomId id);

    // for map
    void sig_showPath(CommandQueue);
    void sig_newRoomSelection(const SigRoomSelection &rs);

    // for user commands
    void sig_command(const QString &, const Coordinate &);

    // for commands that set the mode (emulation, play, map)
    // these are connected to MainWindow
    void sig_setMode(MapModeEnum);

    // emitted when new infomark added by comand
    void sig_infoMarksChanged();

public slots:
    void slot_parseNewUserInput(const TelnetData &);
    void slot_onForcedPositionChange();

    void slot_reset();
    void slot_sendGTellToUser(const QString &, const QString &, const QString &);
    void slot_timersUpdate(const std::string &text);

protected slots:
    void slot_doOfflineCharacterMove();
};
