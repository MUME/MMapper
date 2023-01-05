#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

#include "../configuration/configuration.h"
#include "../expandoracommon/parseevent.h"
#include "../global/StringView.h"
#include "../global/TextUtils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/RoomFieldVariant.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomselection.h"
#include "../pandoragroup/GroupManagerApi.h"
#include "../pandoragroup/mmapper2character.h"
#include "../proxy/ProxyParserApi.h"
#include "../proxy/telnetfilter.h"
#include "../timers/CTimers.h"
#include "../timers/Spells.h"
#include "AbstractParser-Utils.h"
#include "Action.h"
#include "CommandId.h"
#include "CommandQueue.h"
#include "ConnectedRoomFlags.h"
#include "DoorAction.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"

class Coordinate;
class MapData;
class MumeClock;
class ParseEvent;
class Room;
class RoomFieldVariant;
class RoomFilter;
class CTimers;

namespace syntax {
class Sublist;
}

class AbstractParser : public QObject
{
protected:
    static const QString nullString;
    static const QByteArray emptyByteArray;

private:
    Q_OBJECT

protected:
    MumeClock &m_mumeClock;
    CTimers &m_timers;
    Spells &m_spells;

private:
    MapData &m_mapData;
    const ProxyParserApi m_proxy;
    const GroupManagerApi m_group;

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
    const char &prefixChar;

private:
    ActionRecordMap m_actionMap;

protected:
    QString m_exits = nullString;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;
    RoomTerrainEnum m_terrain;

protected:
    CommandEnum m_move = CommandEnum::LOOK;
    QByteArray m_lastPrompt;
    bool m_compactMode = false;
    bool m_overrideSendPrompt = false;
    CommandQueue m_queue;
    bool m_trollExitMapping = false;

private:
    QTimer m_offlineCommandTimer;

public:
    explicit AbstractParser(MapData &, MumeClock &, ProxyParserApi, GroupManagerApi, CTimers &timers, Spells &spells, QObject *parent);
    ~AbstractParser() override;

    void doMove(CommandEnum cmd);
    void sendPromptToUser();
    void sendScoreLineEvent(const QByteArray &arr);
    void sendPromptLineEvent(const QByteArray &arr);

signals:
    // telnet
    void sig_sendToMud(const QByteArray &);
    void sig_sendToUser(const QByteArray &, bool goAhead);
    void sig_mapChanged();
    void sig_graphicsSettingsChanged();
    void sig_releaseAllPaths();

    // used to log
    void sig_log(const QString &, const QString &);

    // for main move/search algorithm
    void sig_handleParseEvent(const SigParseEvent &);

    // for map
    void sig_showPath(CommandQueue);
    void sig_newRoomSelection(const SigRoomSelection &rs);

    // for user commands
    void sig_command(const QByteArray &, const Coordinate &);

    // for commands that set the mode (emulation, play, map)
    // these are connected to MainWindow
    void sig_setMode(MapModeEnum);

    // emitted when new infomark added by comand
    void sig_infoMarksChanged();

public slots:
    void slot_parseNewUserInput(const TelnetData &);

    void slot_reset();
    void slot_sendGTellToUser(const QString &, const QString &, const QString &);
    void slot_timersUpdate(const QString &text);

protected slots:
    void slot_doOfflineCharacterMove();

protected:
    void offlineCharacterMove(CommandEnum direction);
    void offlineCharacterMove() { offlineCharacterMove(CommandEnum::UNKNOWN); }
    void sendRoomInfoToUser(const Room *);
    void sendPromptToUser(const Room &r);
    void sendPromptToUser(char light, char terrain);
    void sendPromptToUser(RoomLightEnum lightType, RoomTerrainEnum terrainType);

    void sendRoomExitsInfoToUser(std::ostream &, const Room *r);
    void sendRoomExitsInfoToUser(const Room *r);
    NODISCARD Coordinate getNextPosition() const;
    NODISCARD Coordinate getTailPosition() const;

    // command handling
    void performDoorCommand(ExitDirEnum direction, DoorActionEnum action);
    void genericDoorCommand(QString command, ExitDirEnum direction, bool locally);

public:
    void setExitFlags(ExitFlags flag, ExitDirEnum dir);
    void setConnectedRoomFlag(DirectSunlightEnum light, ExitDirEnum dir);

    void printRoomInfo(RoomFieldFlags fieldset);
    void printRoomInfo(RoomFieldEnum field);

    void emulateExits(std::ostream &, CommandEnum move);
    NODISCARD QByteArray enhanceExits(const Room *);

    void parseExits(std::ostream &);
    void parsePrompt(const QString &prompt);
    NODISCARD bool parseUserCommands(const QString &command);
    NODISCARD static QString normalizeStringCopy(QString str);

    void searchCommand(const RoomFilter &f);
    void dirsCommand(const RoomFilter &f);

    NODISCARD bool evalActionMap(StringView line);

private:
    void setMode(MapModeEnum mode);
    NODISCARD bool tryParseGenericDoorCommand(const QString &str, bool locally);
    void parseSpecialCommand(StringView);
    NODISCARD bool parseSimpleCommand(const QString &str);

    void showDoorCommandHelp();
    void showMumeTime();
    void showHelp();
    void showMiscHelp();
    void showDoorVariableHelp();
    void showCommandPrefix();
    void showNote();
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
    void doConfig(const StringView &view);
    void doConnectToHost();
    void doDisconnectFromHost();
    void doRemoveDoorNamesCommand();
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
    inline void sendToUser(const QByteArray &arr) { sendToUser(arr, false); }
    inline void sendToUser(const std::string_view s) { sendToUser(::toQByteArrayLatin1(s)); }
    inline void sendToUser(const char *const s) { sendToUser(std::string_view{s}); }
    inline void sendToUser(const QString &s) { sendToUser(s.toLatin1()); }
    friend AbstractParser &operator<<(AbstractParser &self, const std::string_view s)
    {
        self.sendToUser(s);
        return self;
    }
    inline void sendOkToUser() { send_ok(*this); }

protected:
    inline void sendToUser(const QByteArray &arr, const bool goAhead)
    {
        emit sig_sendToUser(arr, goAhead);
    }
    void pathChanged() { emit sig_showPath(m_queue); }
    void mapChanged() { emit sig_mapChanged(); }

protected:
    void log(const QString &a, const QString &b) { emit sig_log(a, b); }

private:
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }
    void sendToMud(const QByteArray &msg) { emit sig_sendToMud(msg); }

private:
    void eval(const std::string &name,
              const std::shared_ptr<const syntax::Sublist> &syntax,
              StringView input);
};
