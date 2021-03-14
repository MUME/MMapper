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
    MumeClock *m_mumeClock = nullptr;

private:
    MapData *m_mapData = nullptr;
    const ProxyParserApi m_proxy;
    const GroupManagerApi m_group;

public:
    using HelpCallback = std::function<void(const std::string &name)>;
    using ParserCallback
        = std::function<bool(const std::vector<StringView> &matched, StringView args)>;
    struct ParserRecord final
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

protected:
    CommandEnum m_move = CommandEnum::LOOK;
    QByteArray m_lastPrompt;
    bool m_compactMode = false;
    bool m_overrideSendPrompt = false;
    CommandQueue m_queue;

private:
    bool m_trollExitMapping = false;
    QTimer m_offlineCommandTimer;

public:
    explicit AbstractParser(
        MapData *, MumeClock *, ProxyParserApi, GroupManagerApi, QObject *parent = nullptr);
    ~AbstractParser() override;

    void doMove(CommandEnum cmd);
    void sendPromptToUser();
    void sendScoreLineEvent(const QByteArray &arr);
    void sendPromptLineEvent(const QByteArray &arr);

signals:
    // telnet
    void sendToMud(const QByteArray &);
    void sig_sendToUser(const QByteArray &, bool goAhead);
    void sig_mapChanged();
    void sig_graphicsSettingsChanged();
    void releaseAllPaths();

    // used to log
    void log(const QString &, const QString &);

    // for main move/search algorithm
    // CAUTION: This hides virtual bool QObject::event(QEvent*).
    void event(const SigParseEvent &);

    // for map
    void showPath(CommandQueue);
    void newRoomSelection(const SigRoomSelection &rs);

    // for user commands
    void command(const QByteArray &, const Coordinate &);

    // for commands that set the mode (emulation, play, map)
    // these are connected to MainWindow
    void setEmulationMode();
    void setPlayMode();
    void setMapMode();

public slots:
    virtual void parseNewMudInput(const TelnetData &) = 0;
    void parseNewUserInput(const TelnetData &);

    void reset();
    void sendGTellToUser(const QString &, const QString &, const QString &);

protected slots:
    void doOfflineCharacterMove();

protected:
    void offlineCharacterMove(CommandEnum direction = CommandEnum::UNKNOWN);
    void sendRoomInfoToUser(const Room *);
    void sendPromptToUser(const Room &r);
    void sendPromptToUser(char light, char terrain);
    void sendPromptToUser(RoomLightEnum lightType, RoomTerrainEnum terrainType);

    void sendRoomExitsInfoToUser(std::ostream &, const Room *r);
    void sendRoomExitsInfoToUser(const Room *r);
    Coordinate getNextPosition() const;
    Coordinate getTailPosition() const;

    // command handling
    void performDoorCommand(ExitDirEnum direction, DoorActionEnum action);
    void genericDoorCommand(QString command, ExitDirEnum direction);

public:
    void setExitFlags(ExitFlags flag, ExitDirEnum dir);
    void setConnectedRoomFlag(DirectSunlightEnum light, ExitDirEnum dir);

    void printRoomInfo(RoomFields fieldset);
    void printRoomInfo(RoomFieldEnum field);

    void emulateExits(std::ostream &, CommandEnum move);
    QByteArray enhanceExits(const Room *);

    void parseExits(std::ostream &);
    void parsePrompt(const QString &prompt);
    virtual bool parseUserCommands(const QString &command);
    static QString normalizeStringCopy(QString str);

    void searchCommand(const RoomFilter &f);
    void dirsCommand(const RoomFilter &f);
    void markCurrentCommand();

    bool evalActionMap(StringView line);

    // these need to be public.  could implement them inline here.
    void doSetEmulationMode();
    void doSetPlayMode();
    void doSetMapMode();

private:
    // NOTE: This declaration only exists to avoid the warning
    // about the "event" signal hiding this function function.
    virtual bool event(QEvent *e) final override { return QObject::event(e); }

    bool tryParseGenericDoorCommand(const QString &str);
    void parseSpecialCommand(StringView);
    bool parseSimpleCommand(const QString &str);

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

    ExitDirEnum tryGetDir(StringView &words);
    void parseSetCommand(StringView view);
    void parseDirections(StringView view);
    void parseSearch(StringView view);

    bool setCommandPrefix(char prefix);

    void openVoteURL();
    void doBackCommand();
    void doConfig(const StringView &view);
    void doConnectToHost();
    void doDisconnectFromHost();
    void doRemoveDoorNamesCommand();
    void doMarkCurrentCommand();
    void doSearchCommand(StringView view);
    void doGetDirectionsCommand(StringView view);
    void toggleTrollMapping();

    void initActionMap();

    void initSpecialCommandMap();
    void addSpecialCommand(const char *s,
                           int minLen,
                           const ParserCallback &callback,
                           const HelpCallback &help);
    bool evalSpecialCommandMap(StringView args);

    void parseHelp(StringView words);
    void parseRoom(StringView input);
    void parseGroup(StringView input);

    bool parseDoorAction(DoorActionEnum dat, StringView words);

public:
    inline void sendToUser(const QByteArray &arr, const bool goAhead)
    {
        emit sig_sendToUser(arr, goAhead);
    }
    inline void sendToUser(const QByteArray &arr) { sendToUser(arr, false); }
    inline void sendToUser(const std::string_view &s) { sendToUser(::toQByteArrayLatin1(s)); }
    inline void sendToUser(const char *const s) { sendToUser(std::string_view{s}); }
    inline void sendToUser(const QString &s) { sendToUser(s.toLatin1()); }
    friend AbstractParser &operator<<(AbstractParser &self, const std::string_view &s)
    {
        self.sendToUser(s);
        return self;
    }
    inline void sendOkToUser() { send_ok(*this); }
    void pathChanged() { emit showPath(m_queue); }
    void mapChanged() { emit sig_mapChanged(); }
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }

private:
    void eval(const std::string &name,
              const std::shared_ptr<const syntax::Sublist> &syntax,
              StringView input);
};
