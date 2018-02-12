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

#include "mmapper2event.h"
#include "telnetfilter.h"

#include <QQueue>
#include <QObject>

class MumeClock;
class ParseEvent;
class MapData;
class Room;
class Coordinate;
class RoomFilter;
class RoomSelection;

typedef QQueue<CommandIdType> CommandQueue;

class AbstractParser : public QObject
{
    Q_OBJECT
public:

    AbstractParser(MapData *, MumeClock *, QObject *parent = 0);
    ~AbstractParser();

    const RoomSelection *search_rs;
signals:
    //telnet
    void sendToMud(const QByteArray &);
    void sendToUser(const QByteArray &);

    void releaseAllPaths();

    //used to log
    void log(const QString &, const QString &);

    //for main move/search algorithm
    void event(ParseEvent * );

    //for map
    void showPath(CommandQueue, bool);

    //for user commands
    void command(const QByteArray &, const Coordinate &);

    //for group manager
    void sendGroupTellEvent(QByteArray);

public slots:
    virtual void parseNewMudInput(IncomingData &) = 0;
    void parseNewUserInput(IncomingData &);

    void reset();
    void emptyQueue();
    void sendGTellToUser(const QByteArray & );

protected:
    void offlineCharacterMove(CommandIdType direction);
    void sendRoomInfoToUser(const Room *);
    void sendPromptToUser();
    void sendPromptToUser(const Room *r);
    void sendRoomExitsInfoToUser(const Room *r);
    const Coordinate getPosition();

    //command handling
    void performDoorCommand(DirectionType direction, DoorActionType action);
    void genericDoorCommand(QString command, DirectionType direction);
    void nameDoorCommand(QString doorname, DirectionType direction);
    void toggleDoorFlagCommand(uint flag, DirectionType direction);
    void toggleExitFlagCommand(uint flag, DirectionType direction);
    void setRoomFieldCommand(const QVariant &flag, uint field);
    void toggleRoomFlagCommand(uint flag, uint field);

    void printRoomInfo(uint fieldset);

    void emulateExits();
    QByteArray enhanceExits(const Room *);

    void parseExits(QString &str);
    void parsePrompt(QString &prompt);
    virtual bool parseUserCommands(QString &command);

    QString m_stringBuffer;
    QByteArray m_newLineTerminator;
    QByteArray m_byteBuffer;

    bool m_readingRoomDesc;
    bool m_descriptionReady;

    QString m_roomName;
    QString m_staticRoomDesc;
    QString m_dynamicRoomDesc;
    ExitsFlagsType m_exitsFlags;
    PromptFlagsType m_promptFlags;
    ConnectedRoomFlagsType m_connectedRoomFlags;
    QString m_lastPrompt;
    bool m_trollExitMapping;

    CommandQueue queue;
    MumeClock *m_mumeClock;
    MapData *m_mapData;

    static const QString nullString;
    static const QString emptyString;
    static const QByteArray emptyByteArray;

    void searchCommand(RoomFilter f);
    void dirsCommand(RoomFilter f);
    void markCurrentCommand();

    DirectionType dirForChar(const QString &dir);
};

#endif
