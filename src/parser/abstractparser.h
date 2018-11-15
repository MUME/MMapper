#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef ABSTRACTPARSER_H
#define ABSTRACTPARSER_H

#include <functional>
#include <map>
#include <string>
#include <QArgument>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVariant>

#include "../expandoracommon/parseevent.h"
#include "../global/StringView.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/mmapper2room.h"
#include "../proxy/telnetfilter.h"
#include "CommandId.h"
#include "ConnectedRoomFlags.h"
#include "DoorAction.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"

class MumeClock;
class ParseEvent;
class MapData;
class Room;
class Coordinate;
class RoomFilter;
class RoomSelection;

using CommandQueue = QQueue<CommandIdType>;

class AbstractParser : public QObject
{
protected:
    static const QString nullString;
    static const QString emptyString;
    static const QByteArray emptyByteArray;

private:
    Q_OBJECT

protected:
    MumeClock *m_mumeClock = nullptr;

private:
    MapData *m_mapData = nullptr;
    const RoomSelection *search_rs = nullptr;

private:
    using HelpCallback = std::function<void(const std::string &name)>;
    using ParserCallback
        = std::function<bool(const std::vector<StringView> &matched, StringView args)>;
    struct ParserRecord final
    {
        std::string fullCommand;
        ParserCallback callback;
        HelpCallback help;
    };
    std::map<std::string, ParserRecord> m_specialCommandMap{};
    QByteArray m_newLineTerminator{};
    char prefixChar = '_';

protected:
    QString m_exits = nullString;
    ExitsFlagsType m_exitsFlags{};
    PromptFlagsType m_promptFlags{};
    ConnectedRoomFlagsType m_connectedRoomFlags{};

protected:
    QByteArray m_lastPrompt{};
    bool m_compactMode = false;
    bool m_overrideSendPrompt = false;
    CommandQueue queue{};

private:
    bool m_trollExitMapping = false;
    QTimer m_offlineCommandTimer;

public:
    explicit AbstractParser(MapData *, MumeClock *, QObject *parent = nullptr);
    ~AbstractParser() override;

signals:
    // telnet
    void sendToMud(const QByteArray &);
    void sig_sendToUser(const QByteArray &, bool goAhead);
    void releaseAllPaths();

    // used to log
    void log(const QString &, const QString &);

    // for main move/search algorithm
    // CAUTION: This hides virtual bool QObject::event(QEvent*).
    void event(const SigParseEvent &);

    // for map
    void showPath(CommandQueue, bool);

    // for user commands
    void command(const QByteArray &, const Coordinate &);

    // for group manager
    void sendGroupTellEvent(QByteArray);

public slots:
    virtual void parseNewMudInput(const IncomingData &) = 0;
    void parseNewUserInput(const IncomingData &);

    void reset();
    void sendGTellToUser(const QByteArray &);

protected slots:
    void doOfflineCharacterMove();

protected:
    void offlineCharacterMove(CommandIdType direction);
    void sendRoomInfoToUser(const Room *);
    void sendPromptToUser();
    void sendPromptToUser(const Room &r);
    void sendPromptToUser(char light, char terrain);
    void sendPromptToUser(RoomLightType lightType, RoomTerrainType terrainType);

    void sendRoomExitsInfoToUser(const Room *r);
    const Coordinate getPosition();

    // command handling
    void performDoorCommand(DirectionType direction, DoorActionType action);
    void genericDoorCommand(QString command, DirectionType direction);
    void nameDoorCommand(const QString &doorname, DirectionType direction);
    void toggleDoorFlagCommand(DoorFlag flag, DirectionType direction);
    void toggleExitFlagCommand(ExitFlag flag, DirectionType direction);
    [[deprecated]] void setRoomFieldCommand(const QVariant &flag, RoomField field);
    void setRoomFieldCommand(RoomAlignType rat, RoomField field);
    void setRoomFieldCommand(RoomLightType rlt, RoomField field);
    void setRoomFieldCommand(RoomPortableType rpt, RoomField field);
    void setRoomFieldCommand(RoomRidableType rrt, RoomField field);
    void setRoomFieldCommand(RoomSundeathType rst, RoomField field);

    ExitFlags getExitFlags(DirectionType dir) const;
    DirectionalLightType getConnectedRoomFlags(DirectionType dir) const;
    void setExitFlags(ExitFlags flag, DirectionType dir);
    void setConnectedRoomFlag(DirectionalLightType light, DirectionType dir);

    [[deprecated]] void toggleRoomFlagCommand(uint flag, RoomField field);
    void toggleRoomFlagCommand(RoomMobFlag flag, RoomField field);
    void toggleRoomFlagCommand(RoomLoadFlag flag, RoomField field);

    void printRoomInfo(RoomFields fieldset);
    void printRoomInfo(RoomField field);

    void emulateExits();
    QByteArray enhanceExits(const Room *);

    void parseExits();
    void parsePrompt(const QString &prompt);
    virtual bool parseUserCommands(const QString &command);
    static QString normalizeStringCopy(QString str);

    void searchCommand(const RoomFilter &f);
    void dirsCommand(const RoomFilter &f);
    void markCurrentCommand();

private:
    // NOTE: This declaration only exists to avoids the warning
    // about the "event" signal hiding this function function.
    virtual bool event(QEvent *e) final override { return QObject::event(e); }

    bool tryParseGenericDoorCommand(const QString &str);
    void parseSpecialCommand(StringView);
    bool parseSimpleCommand(const QString &str);

    void showDoorCommandHelp();
    void showMumeTime();
    void showHelp();
    void showMapHelp();
    void showGroupHelp();
    void showExitHelp();
    void showRoomSimpleFlagsHelp();
    void showRoomMobFlagsHelp();
    void showRoomLoadFlagsHelp();
    void showMiscHelp();
    void showDoorFlagHelp();
    void showExitFlagHelp();
    void showDoorVariableHelp();
    void showCommandPrefix();
    void showNote();
    void showSyntax(const char *rest);
    void showHelpCommands(bool showAbbreviations);

    void showHeader(const QString &s);

    bool getField(const Coordinate &c,
                  const DirectionType &direction,
                  const ExitFieldVariant &var) const;

    DirectionType tryGetDir(StringView &words);
    bool parseDoorAction(StringView words);
    bool parseDoorFlags(StringView words);
    bool parseExitFlags(StringView words);
    bool parseField(const StringView view);
    bool parseMobFlags(const StringView view);
    bool parseLoadFlags(const StringView view);
    void parseSetCommand(StringView view);
    void parseName(StringView view);
    void parseNoteCmd(StringView view);
    void parseDirections(StringView view);
    void parseSearch(StringView view);
    void parseGtell(const StringView &view);

    bool setCommandPrefix(char prefix);
    void setNote(const QString &note);

    void openVoteURL();
    void doBackCommand();
    void doRemoveDoorNamesCommand();
    void doMarkCurrentCommand();
    void doSearchCommand(StringView view);
    void doGetDirectionsCommand(StringView view);
    void toggleTrollMapping();

    void initSpecialCommandMap();
    void addSpecialCommand(const char *s,
                           int minLen,
                           const ParserCallback &callback,
                           const HelpCallback &help);
    bool evalSpecialCommandMap(StringView args);

    void parseHelp(StringView words);
    bool parsePrint(StringView &input);

    bool parseDoorAction(DoorActionType dat, StringView words);
    bool parseDoorFlag(DoorFlag flag, StringView words);
    bool parseExitFlag(ExitFlag flag, StringView words);

    void doMove(CommandIdType cmd);

public:
    inline void sendToUser(const QByteArray &arr, bool goAhead = false)
    {
        emit sig_sendToUser(arr, goAhead);
    }
    inline void sendToUser(const char *s, bool goAhead = false)
    {
        sendToUser(QByteArray{s}, goAhead);
    }
    inline void sendToUser(const QString &s, bool goAhead = false)
    {
        sendToUser(s.toLatin1(), goAhead);
    }
};

#endif
