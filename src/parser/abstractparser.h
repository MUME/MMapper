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

#include "telnetfilter.h"
#include "defs.h"

#include <QQueue>
#include <QObject>

class ParseEvent;
class MapData;
class Room;
class Coordinate;

enum CommandIdType   { CID_NORTH = 0, CID_SOUTH, CID_EAST, CID_WEST, CID_UP, CID_DOWN,
						CID_UNKNOWN, CID_LOOK, CID_FLEE, CID_SCOUT, /*CID_SYNC, CID_RESET, */CID_NONE };

enum DoorActionType { DAT_OPEN, DAT_CLOSE, DAT_LOCK, DAT_UNLOCK, DAT_PICK, DAT_ROCK, DAT_BASH, DAT_BREAK, DAT_BLOCK, DAT_NONE };

#define EXIT_N bit1
#define EXIT_S bit5
#define EXIT_E bit9
#define EXIT_W bit13
#define EXIT_U bit17
#define EXIT_D bit21

#define DOOR_N bit2
#define DOOR_S bit6
#define DOOR_E bit10
#define DOOR_W bit14
#define DOOR_U bit18
#define DOOR_D bit22

#define ROAD_N bit3
#define ROAD_S bit7
#define ROAD_E bit11
#define ROAD_W bit15
#define ROAD_U bit19
#define ROAD_D bit23

#define CLIMB_N bit4
#define CLIMB_S bit8
#define CLIMB_E bit12
#define CLIMB_W bit16
#define CLIMB_U bit20
#define CLIMB_D bit24

#define EXITS_FLAGS_VALID bit25
typedef quint32 ExitsFlagsType;

// 0-3 terrain type
#define PROMPT_FLAGS_VALID bit8
#define SUN_ROOM bit9
typedef quint16 PromptFlagsType;

typedef QQueue<CommandIdType> CommandQueue;

class AbstractParser : public QObject
{
  Q_OBJECT
public:

   AbstractParser(MapData*, QObject *parent = 0);

signals:
  //telnet
  void sendToMud(const QByteArray&);
  void sendToUser(const QByteArray&);

  void setNormalMode();
  void setXmlMode();

  void releaseAllPaths();

  //used to log
  void logText( const QString& );

  //for main move/search algorithm
  void event(ParseEvent* );

  //for map
  void showPath(CommandQueue, bool);

  //for user commands
  void command(const QByteArray&, const Coordinate &);

  //for group manager
  void sendPromptLineEvent(QByteArray);
  void sendGroupTellEvent(QByteArray);

public slots:
  virtual void parseNewMudInput(IncomingData&) = 0;
  void parseNewUserInput(IncomingData&);

  void emptyQueue();

protected:
  //for main move/search algorithm
  void characterMoved(CommandIdType, const QString&, const QString&, const QString&, ExitsFlagsType, PromptFlagsType);
  void offlineCharacterMove(CommandIdType direction);
  void sendRoomInfoToUser(const Room*);
  void sendPromptSimulationToUser();
  void sendRoomExitsInfoToUser(const Room* r);

  //command handling
  void performDoorCommand(DirectionType direction, DoorActionType action);
  void genericDoorCommand(QString command, DirectionType direction);
  void nameDoorCommand(QString doorname, DirectionType direction);
  void toggleDoorFlagCommand(uint flag, DirectionType direction);
  void toggleExitFlagCommand(uint flag, DirectionType direction);
  void setRoomFieldCommand(uint flag, uint field);
  void toggleRoomFlagCommand(uint flag, uint field);

  //utility functions
  QString& removeAnsiMarks(QString& str);

  void emulateExits();

  void parseExits(QString& str);
  void parsePrompt(QString& prompt);
  virtual bool parseUserCommands(QString& command);

  static const QChar escChar;

  QString m_stringBuffer;
  QByteArray m_byteBuffer;

  bool m_readingRoomDesc;
  bool m_descriptionReady;

  bool m_examine;

  QString m_roomName;
  QString m_staticRoomDesc;
  QString m_dynamicRoomDesc;
  ExitsFlagsType m_exitsFlags;
  PromptFlagsType m_promptFlags;

  CommandQueue queue;

  MapData* m_mapData;

  static const QString nullString;
  static const QString emptyString;
  static const QByteArray emptyByteArray;

  QString& latinToAscii(QString& str);
};

#endif
