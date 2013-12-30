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

#include "abstractparser.h"
#include "mmapper2event.h"
#include "mmapper2room.h"
#include "mmapper2exit.h"
#include "configuration.h"
#include "roomselection.h"
#include "mapdata.h"

const QChar AbstractParser::escChar('\x1B');
const QString AbstractParser::nullString;
const QString AbstractParser::emptyString("");
const QByteArray AbstractParser::emptyByteArray("");

AbstractParser::AbstractParser(MapData* md, QObject *parent)
  : QObject(parent)
{
  m_readingRoomDesc = false;
  m_descriptionReady = false;
  m_examine = false;

  m_mapData = md;
   /*m_roomName = "";
  m_dynamicRoomDesc = "";
  m_staticRoomDesc = "";*/
  m_exitsFlags = 0;
  m_promptFlags = 0;
}

void AbstractParser::characterMoved(CommandIdType c, const QString& roomName, const QString& dynamicRoomDesc, const QString& staticRoomDesc, ExitsFlagsType exits, PromptFlagsType prompt)
{
  emit event(createEvent(c, roomName, dynamicRoomDesc, staticRoomDesc, exits, prompt));
}

void AbstractParser::emptyQueue()
{
  queue.clear();
}

QString& AbstractParser::removeAnsiMarks(QString& str) {
  static const QChar colorEndMark('m');
  QString out=emptyString;
  bool started=FALSE;

  for ( int i=0; i< str.length(); i++ ){
    if ( started && (str.at(i) == colorEndMark)){
      started = FALSE;
      continue;
    }
    if (str.at(i) == escChar){
      started = TRUE;
      continue;
    }
    if (started) continue;
    out.append((str.at(i).toAscii()));
  }
  str = out;
  return str;
}

void AbstractParser::parsePrompt(QString& prompt){
  m_promptFlags = 0;
  quint8 index = 0;
  int sv;

  emit sendPromptLineEvent(m_stringBuffer.toAscii());

  switch (sv=(int)((prompt[index]).toAscii())) {
  case 64: index++;m_promptFlags=SUN_ROOM;break; // @  direct sunlight
  case 42: index++;m_promptFlags=SUN_ROOM;break; // *  indoor/cloudy sun
  case 33: index++;break;                        // !  artifical light
  case 41: index++;m_promptFlags=SUN_ROOM;break; // )  moonlight
  case 111:index++;break;                        // o  darkness
  default:;
  }

  switch ( sv = (int)(prompt[index]).toAscii() )
  {
    case 91: SET(m_promptFlags,RTT_INDOORS); break; // [  // indoors
    case 35: SET(m_promptFlags,RTT_CITY); break; // #  // city
    case 46: SET(m_promptFlags,RTT_FIELD); break; // .  // field
    case 102: SET(m_promptFlags,RTT_FOREST); break; // f  // forest
    case 40: SET(m_promptFlags,RTT_HILLS); break; // (  // hills
    case 60: SET(m_promptFlags,RTT_MOUNTAINS); break; // <  // mountains
    case 37: SET(m_promptFlags,RTT_SHALLOW); break; // %  // shallow
    case 126:SET(m_promptFlags,RTT_WATER); break; // ~w  // water
    case 87: SET(m_promptFlags,RTT_RAPIDS); break; // W  // rapids
    case 85: SET(m_promptFlags,RTT_UNDERWATER); break; // U  // underwater
    case 43: SET(m_promptFlags,RTT_ROAD); break; // +  // road
    case 61: SET(m_promptFlags,RTT_TUNNEL); break; // =  // tunnel
    case 79: SET(m_promptFlags,RTT_CAVERN); break; // O  // cavern
    case 58: SET(m_promptFlags,RTT_BRUSH); break; // :  // brush
    default:;
  }
  SET(m_promptFlags,PROMPT_FLAGS_VALID);
}

void AbstractParser::parseExits(QString& str)
{
  m_exitsFlags = 0;
  bool doors=FALSE;
  bool road=FALSE;
  bool climb=FALSE;
  bool portal=FALSE;

  int length = str.length();
  for (int i=7; i<length; i++){
    switch ((int)(str.at(i).toAscii())){
    case 40: doors=true;break;    /* ( */
    case 91: doors=true;break;    /* [ */
    case 35: doors=true;break;    /* # */
    case 61: road=true;break;     /* = */
    case 45: road=true;break;     /* - */
    case 47: climb=true;break;    /* / */
    case 92: climb=true;break;    /* \ */
    case 123: portal=true;break;  /* { */

    case 110:  // n
      if ( (i+2)<length && (str.at(i+2).toAscii()) == 'r') //north
        {
          i+=5;
	  if (climb) {
	    SET(m_exitsFlags,CLIMB_N);
            SET(m_exitsFlags,EXIT_N);
	    climb=false;
	  } else if (doors){
            SET(m_exitsFlags,DOOR_N);
            SET(m_exitsFlags,EXIT_N);
            doors=false;
          }else
            SET(m_exitsFlags,EXIT_N);
	  if (road){
	    SET(m_exitsFlags,ROAD_N);
	    road=false;
	  }
        }
      else
	i+=4;  //none
      break;

    case 115:  // s
      i+=5;
      if (climb) {
	SET(m_exitsFlags,CLIMB_S);
	SET(m_exitsFlags,EXIT_S);
	climb=false;
      } else if (doors){
	SET(m_exitsFlags,DOOR_S);
	SET(m_exitsFlags,EXIT_S);
	doors=false;
      }else
	SET(m_exitsFlags,EXIT_S);
      if (road){
	SET(m_exitsFlags,ROAD_S);
	road=false;
      }
      break;

    case 101:  // e
      i+=4;
      if (climb) {
	SET(m_exitsFlags,CLIMB_E);
	SET(m_exitsFlags,EXIT_E);
	climb=false;
      } else if (doors){
	SET(m_exitsFlags,DOOR_E);
	SET(m_exitsFlags,EXIT_E);
	doors=false;
      }else
	SET(m_exitsFlags,EXIT_E);
      if (road){
	SET(m_exitsFlags,ROAD_E);
	road=false;
      }
      break;

    case 119:  // w
      i+=4;
      if (climb) {
	SET(m_exitsFlags,CLIMB_W);
	SET(m_exitsFlags,EXIT_W);
	climb=false;
      } else if (doors){
	SET(m_exitsFlags,DOOR_W);
	SET(m_exitsFlags,EXIT_W);
	doors=false;
      }else
	SET(m_exitsFlags,EXIT_W);
      if (road){
	SET(m_exitsFlags,ROAD_W);
	road=false;
      }
      break;

    case 117:  // u
      i+=2;
      if (climb) {
	SET(m_exitsFlags,CLIMB_U);
	SET(m_exitsFlags,EXIT_U);
	climb=false;
      } else if (doors){
	SET(m_exitsFlags,DOOR_U);
	SET(m_exitsFlags,EXIT_U);
	doors=false;
      }else
	SET(m_exitsFlags,EXIT_U);
      if (road){
	SET(m_exitsFlags,ROAD_U);
	road=false;
      }
      break;

    case 100:  // d
      i+=4;
      if (climb) {
	SET(m_exitsFlags,CLIMB_D);
	SET(m_exitsFlags,EXIT_D);
	climb=false;
      } else if (doors){
	SET(m_exitsFlags,DOOR_D);
	SET(m_exitsFlags,EXIT_D);
	doors=false;
      }else
	SET(m_exitsFlags,EXIT_D);
      if (road){
	SET(m_exitsFlags,ROAD_D);
	road=false;
      }
      break;
    default:;
    }
  }

  // If there is't a portal then we can trust the exits
  if (!portal) SET(m_exitsFlags, EXITS_FLAGS_VALID);

  Coordinate c;
  QByteArray dn = emptyByteArray;
  QByteArray cn = " -";

  CommandQueue tmpqueue;
  bool noDoors = true;

  if (!queue.isEmpty())
    tmpqueue.enqueue(queue.head());

  QList<Coordinate> cl = m_mapData->getPath(tmpqueue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  for (uint i=0;i<6;i++)
    {
      dn = m_mapData->getDoorName(c, i).toAscii();
      if ( dn != emptyByteArray )
	{
	  noDoors = false;
	  switch (i)
	    {
	    case 0:
	      cn += " n:"+dn;
	      break;
	    case 1:
	      cn += " s:"+dn;
	      break;
	    case 2:
	      cn += " e:"+dn;
	      break;
	    case 3:
	      cn += " w:"+dn;
	      break;
	    case 4:
	      cn += " u:"+dn;
	      break;
	    case 5:
	      cn += " d:"+dn;
	      break;
	    default:
	      break;
	    }
	}
    }

  if (noDoors)
    {
      cn = "\r\n";
    }
  else
    {
      cn += ".\r\n";

    }

  emit sendToUser(str.toAscii()+cn);
  //emit sendToUser(str.toAscii()+QByteArray("\r\n"));
  //emit sendToUser(cn);
}

void AbstractParser::emulateExits()
{
  Coordinate c;
//    QByteArray dn = "";
//    QByteArray cn = "Exits: ";

  CommandQueue tmpqueue;
//      bool noDoors = true;

  if (!queue.isEmpty())
    tmpqueue.enqueue(queue.head());

  QList<Coordinate> cl = m_mapData->getPath(tmpqueue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  const RoomSelection * rs = m_mapData->select();
  const Room *r = m_mapData->getRoom(c, rs);

  sendRoomExitsInfoToUser(r);

  m_mapData->unselect(rs);

        /*
  for (uint i=0;i<6;i++)
  {
  dn = m_mapData->getDoorName(c, i).toAscii();
  if ( dn != "" )
  {
  noDoors = false;
  switch (i)
  {
  case 0:
  cn += " n:"+dn;
  break;
  case 1:
  cn += " s:"+dn;
  break;
  case 2:
  cn += " e:"+dn;
  break;
  case 3:
  cn += " w:"+dn;
  break;
  case 4:
  cn += " u:"+dn;
  break;
  case 5:
  cn += " d:"+dn;
  break;
  default:
  break;
}
}
}

  if (noDoors)
  {
  cn = "\r\n";
}
  else
  {
  cn += ".\r\n";

}

  emit sendToUser(str.toAscii()+cn);
        //emit sendToUser(str.toAscii()+QByteArray("\r\n"));
  //emit sendToUser(cn);  */

}


void AbstractParser::parseNewUserInput(IncomingData& data)
{
  switch (data.type)
  {
    case TDT_DELAY:
    case TDT_PROMPT:
    case TDT_MENU_PROMPT:
    case TDT_LOGIN:
    case TDT_LOGIN_PASSWORD:
    case TDT_TELNET:
    case TDT_SPLIT:
    case TDT_UNKNOWN:
      emit sendToMud(data.line);
      break;
    case TDT_CRLF:
      m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
      m_stringBuffer = m_stringBuffer.simplified();
      if (parseUserCommands(m_stringBuffer))
        emit sendToMud(data.line);
                                //else
                                        //emit sendToMud(QByteArray("\r\n"));
      break;
    case TDT_LFCR:
      m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
      m_stringBuffer = m_stringBuffer.simplified();
      if (parseUserCommands(m_stringBuffer))
        emit sendToMud(data.line);
                                //else
                                        //emit sendToMud(QByteArray("\r\n"));
      break;
    case TDT_LF:
      m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
      m_stringBuffer = m_stringBuffer.simplified();
      if ( parseUserCommands(m_stringBuffer))
        emit sendToMud(data.line);
                                //else
                                        //emit sendToMud(QByteArray("\r\n"));
      break;
  }
}


bool AbstractParser::parseUserCommands(QString& command)
{
  QString str = command;

  DoorActionType daction = DAT_NONE;
  bool dooraction = FALSE;

  if (str.contains("$$DOOR"))
  {
    if (str.contains("$$DOOR_N$$")){genericDoorCommand(command, NORTH); return false;}
    else
      if (str.contains("$$DOOR_S$$")){genericDoorCommand(command, SOUTH); return false;}
    else
      if (str.contains("$$DOOR_E$$")){genericDoorCommand(command, EAST); return false;}
    else
      if (str.contains("$$DOOR_W$$")){genericDoorCommand(command, WEST); return false;}
    else
      if (str.contains("$$DOOR_U$$")){genericDoorCommand(command, UP); return false;}
    else
      if (str.contains("$$DOOR_D$$")){genericDoorCommand(command, DOWN); return false;}
    else
      if (str.contains("$$DOOR$$"))  {genericDoorCommand(command, UNKNOWN); return false;}
  }

  if (str.startsWith('_'))
  {
    if (str.startsWith("_gt")) // grouptell
    {
      if (!str.section(" ",1,1).toAscii().isEmpty()) {
        QByteArray data = str.remove(0, 4).toAscii(); // 3 is length of "_gt "
        //qDebug( "Sending a G-tell from local user: %s", (const char *) data);
        emit sendGroupTellEvent(data);
      }
      return false;
    }
    else if (str.startsWith("_brief"))
    {
      if (Config().m_brief == TRUE)
      {
        Config().m_brief = FALSE;
        emit sendToUser((QByteArray)"[MMapper] brief mode off.\r\n");
        return false;
      }
      if (Config().m_brief == FALSE)
      {
        Config().m_brief = TRUE;
        emit sendToUser((QByteArray)"[MMapper] brief mode on.\r\n");
        return false;
      }
    }
    else if (str.startsWith("_name"))
    {
      if (str.section(" ",1,1)=="n") nameDoorCommand(str.section(" ",2,2), NORTH);
      if (str.section(" ",1,1)=="s") nameDoorCommand(str.section(" ",2,2), SOUTH);
      if (str.section(" ",1,1)=="e") nameDoorCommand(str.section(" ",2,2), EAST);
      if (str.section(" ",1,1)=="w") nameDoorCommand(str.section(" ",2,2), WEST);
      if (str.section(" ",1,1)=="u") nameDoorCommand(str.section(" ",2,2), UP);
      if (str.section(" ",1,1)=="d") nameDoorCommand(str.section(" ",2,2), DOWN);
      if (str.section(" ",1,1)=="s") nameDoorCommand(str.section(" ",2,2), SOUTH);
      return false;
    }
    else if (str.startsWith("_door") && str != "_doorhelp")
    {
      if (!str.section(" ",2,2).toAscii().isEmpty())
        emit sendToUser("--->Incorrect Command. Perhaps you meant _name?\n\r");
      else {
        if (str.section(" ",1,1)=="n") toggleExitFlagCommand(EF_DOOR, NORTH);
        if (str.section(" ",1,1)=="s") toggleExitFlagCommand(EF_DOOR, SOUTH);
        if (str.section(" ",1,1)=="e") toggleExitFlagCommand(EF_DOOR, EAST);
        if (str.section(" ",1,1)=="w") toggleExitFlagCommand(EF_DOOR, WEST);
        if (str.section(" ",1,1)=="u") toggleExitFlagCommand(EF_DOOR, UP);
        if (str.section(" ",1,1)=="d") toggleExitFlagCommand(EF_DOOR, DOWN);
      }
      return false;
    }
    else if (str.startsWith("_hidden"))
    {
      if (str.section(" ",1,1)=="n") toggleDoorFlagCommand(DF_HIDDEN, NORTH);
      if (str.section(" ",1,1)=="s") toggleDoorFlagCommand(DF_HIDDEN, SOUTH);
      if (str.section(" ",1,1)=="e") toggleDoorFlagCommand(DF_HIDDEN, EAST);
      if (str.section(" ",1,1)=="w") toggleDoorFlagCommand(DF_HIDDEN, WEST);
      if (str.section(" ",1,1)=="u") toggleDoorFlagCommand(DF_HIDDEN, UP);
      if (str.section(" ",1,1)=="d") toggleDoorFlagCommand(DF_HIDDEN, DOWN);
      return false;
    }
    else  if (str.startsWith("_needkey"))
    {
      if (str.section(" ",1,1)=="n") toggleDoorFlagCommand(DF_NEEDKEY, NORTH);
      if (str.section(" ",1,1)=="s") toggleDoorFlagCommand(DF_NEEDKEY, SOUTH);
      if (str.section(" ",1,1)=="e") toggleDoorFlagCommand(DF_NEEDKEY, EAST);
      if (str.section(" ",1,1)=="w") toggleDoorFlagCommand(DF_NEEDKEY, WEST);
      if (str.section(" ",1,1)=="u") toggleDoorFlagCommand(DF_NEEDKEY, UP);
      if (str.section(" ",1,1)=="d") toggleDoorFlagCommand(DF_NEEDKEY, DOWN);
      return false;
    }
    else if (str.startsWith("_noblock"))
    {
      if (str.section(" ",1,1)=="n") toggleDoorFlagCommand(DF_NOBLOCK, NORTH);
      if (str.section(" ",1,1)=="s") toggleDoorFlagCommand(DF_NOBLOCK, SOUTH);
      if (str.section(" ",1,1)=="e") toggleDoorFlagCommand(DF_NOBLOCK, EAST);
      if (str.section(" ",1,1)=="w") toggleDoorFlagCommand(DF_NOBLOCK, WEST);
      if (str.section(" ",1,1)=="u") toggleDoorFlagCommand(DF_NOBLOCK, UP);
      if (str.section(" ",1,1)=="d") toggleDoorFlagCommand(DF_NOBLOCK, DOWN);
      return false;
    }
    else if (str.startsWith("_nobreak"))
    {
      if (str.section(" ",1,1)=="n") toggleDoorFlagCommand(DF_NOBREAK, NORTH);
      if (str.section(" ",1,1)=="s") toggleDoorFlagCommand(DF_NOBREAK, SOUTH);
      if (str.section(" ",1,1)=="e") toggleDoorFlagCommand(DF_NOBREAK, EAST);
      if (str.section(" ",1,1)=="w") toggleDoorFlagCommand(DF_NOBREAK, WEST);
      if (str.section(" ",1,1)=="u") toggleDoorFlagCommand(DF_NOBREAK, UP);
      if (str.section(" ",1,1)=="d") toggleDoorFlagCommand(DF_NOBREAK, DOWN);
      return false;
    }
    else if (str.startsWith("_nopick"))
    {
      if (str.section(" ",1,1)=="n") toggleDoorFlagCommand(DF_NOPICK, NORTH);
      if (str.section(" ",1,1)=="s") toggleDoorFlagCommand(DF_NOPICK, SOUTH);
      if (str.section(" ",1,1)=="e") toggleDoorFlagCommand(DF_NOPICK, EAST);
      if (str.section(" ",1,1)=="w") toggleDoorFlagCommand(DF_NOPICK, WEST);
      if (str.section(" ",1,1)=="u") toggleDoorFlagCommand(DF_NOPICK, UP);
      if (str.section(" ",1,1)=="d") toggleDoorFlagCommand(DF_NOPICK, DOWN);
      return false;
    }
    else if (str.startsWith("_delayed"))
    {
      if (str.section(" ",1,1)=="n") toggleDoorFlagCommand(DF_DELAYED, NORTH);
      if (str.section(" ",1,1)=="s") toggleDoorFlagCommand(DF_DELAYED, SOUTH);
      if (str.section(" ",1,1)=="e") toggleDoorFlagCommand(DF_DELAYED, EAST);
      if (str.section(" ",1,1)=="w") toggleDoorFlagCommand(DF_DELAYED, WEST);
      if (str.section(" ",1,1)=="u") toggleDoorFlagCommand(DF_DELAYED, UP);
      if (str.section(" ",1,1)=="d") toggleDoorFlagCommand(DF_DELAYED, DOWN);
      return false;
    }
    else if (str.startsWith("_exit"))
    {
      if (str.section(" ",1,1)=="n") toggleExitFlagCommand(EF_EXIT, NORTH);
      if (str.section(" ",1,1)=="s") toggleExitFlagCommand(EF_EXIT, SOUTH);
      if (str.section(" ",1,1)=="e") toggleExitFlagCommand(EF_EXIT, EAST);
      if (str.section(" ",1,1)=="w") toggleExitFlagCommand(EF_EXIT, WEST);
      if (str.section(" ",1,1)=="u") toggleExitFlagCommand(EF_EXIT, UP);
      if (str.section(" ",1,1)=="d") toggleExitFlagCommand(EF_EXIT, DOWN);
      return false;
    }
    else if (str.startsWith("_road"))
    {
      if (str.section(" ",1,1)=="n") toggleExitFlagCommand(EF_ROAD, NORTH);
      if (str.section(" ",1,1)=="s") toggleExitFlagCommand(EF_ROAD, SOUTH);
      if (str.section(" ",1,1)=="e") toggleExitFlagCommand(EF_ROAD, EAST);
      if (str.section(" ",1,1)=="w") toggleExitFlagCommand(EF_ROAD, WEST);
      if (str.section(" ",1,1)=="u") toggleExitFlagCommand(EF_ROAD, UP);
      if (str.section(" ",1,1)=="d") toggleExitFlagCommand(EF_ROAD, DOWN);
      return false;
    }
    else if (str.startsWith("_climb"))
    {
      if (str.section(" ",1,1)=="n") toggleExitFlagCommand(EF_CLIMB, NORTH);
      if (str.section(" ",1,1)=="s") toggleExitFlagCommand(EF_CLIMB, SOUTH);
      if (str.section(" ",1,1)=="e") toggleExitFlagCommand(EF_CLIMB, EAST);
      if (str.section(" ",1,1)=="w") toggleExitFlagCommand(EF_CLIMB, WEST);
      if (str.section(" ",1,1)=="u") toggleExitFlagCommand(EF_CLIMB, UP);
      if (str.section(" ",1,1)=="d") toggleExitFlagCommand(EF_CLIMB, DOWN);
      return false;
    }
    else if (str.startsWith("_random"))
    {
      if (str.section(" ",1,1)=="n") toggleExitFlagCommand(EF_RANDOM, NORTH);
      if (str.section(" ",1,1)=="s") toggleExitFlagCommand(EF_RANDOM, SOUTH);
      if (str.section(" ",1,1)=="e") toggleExitFlagCommand(EF_RANDOM, EAST);
      if (str.section(" ",1,1)=="w") toggleExitFlagCommand(EF_RANDOM, WEST);
      if (str.section(" ",1,1)=="u") toggleExitFlagCommand(EF_RANDOM, UP);
      if (str.section(" ",1,1)=="d") toggleExitFlagCommand(EF_RANDOM, DOWN);
      return false;
    }
    else if (str.startsWith("_special"))
    {
      if (str.section(" ",1,1)=="n") toggleExitFlagCommand(EF_SPECIAL, NORTH);
      if (str.section(" ",1,1)=="s") toggleExitFlagCommand(EF_SPECIAL, SOUTH);
      if (str.section(" ",1,1)=="e") toggleExitFlagCommand(EF_SPECIAL, EAST);
      if (str.section(" ",1,1)=="w") toggleExitFlagCommand(EF_SPECIAL, WEST);
      if (str.section(" ",1,1)=="u") toggleExitFlagCommand(EF_SPECIAL, UP);
      if (str.section(" ",1,1)=="d") toggleExitFlagCommand(EF_SPECIAL, DOWN);
      return false;
    }
    else if (str.startsWith("_lit"))
    {
      setRoomFieldCommand(RLT_LIT, R_LIGHTTYPE);
      return false;
    }
    else if (str.startsWith("_dark"))
    {
      setRoomFieldCommand(RLT_DARK, R_LIGHTTYPE);
      return false;
    }
    else if (str.startsWith("_port"))
    {
      setRoomFieldCommand(RPT_PORTABLE, R_PORTABLETYPE);
      return false;
    }
    else if (str.startsWith("_noport"))
    {
      setRoomFieldCommand(RPT_NOTPORTABLE, R_PORTABLETYPE);
      return false;
    }
    else if (str.startsWith("_ride"))
    {
      setRoomFieldCommand(RRT_RIDABLE, R_RIDABLETYPE);
      return false;
    }
    else if (str.startsWith("_noride"))
    {
      setRoomFieldCommand(RRT_NOTRIDABLE, R_RIDABLETYPE);
      return false;
    }
    else if (str.startsWith("_good"))
    {
      setRoomFieldCommand(RAT_GOOD, R_ALIGNTYPE);
      return false;
    }
    else if (str.startsWith("_neutral"))
    {
      setRoomFieldCommand(RAT_NEUTRAL, R_ALIGNTYPE);
      return false;
    }
    else if (str.startsWith("_evil"))
    {
      setRoomFieldCommand(RAT_EVIL, R_ALIGNTYPE);
      return false;
    }
    else if (str.startsWith("_rent"))
    {
      toggleRoomFlagCommand(RMF_RENT, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_shop"))
    {
      toggleRoomFlagCommand(RMF_SHOP, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_weaponshop"))
    {
      toggleRoomFlagCommand(RMF_WEAPONSHOP, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_armourshop"))
    {
      toggleRoomFlagCommand(RMF_ARMOURSHOP, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_foodshop"))
    {
      toggleRoomFlagCommand(RMF_FOODSHOP, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_petshop"))
    {
      toggleRoomFlagCommand(RMF_PETSHOP, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_guild"))
    {
      toggleRoomFlagCommand(RMF_GUILD, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_scoutguild"))
    {
      toggleRoomFlagCommand(RMF_SCOUTGUILD, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_mageguild"))
    {
      toggleRoomFlagCommand(RMF_MAGEGUILD, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_clericguild"))
    {
      toggleRoomFlagCommand(RMF_CLERICGUILD, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_warriorguild"))
    {
      toggleRoomFlagCommand(RMF_WARRIORGUILD, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_rangerguild"))
    {
      toggleRoomFlagCommand(RMF_RANGERGUILD, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_smob"))
    {
      toggleRoomFlagCommand(RMF_SMOB, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_quest"))
    {
      toggleRoomFlagCommand(RMF_QUEST, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_mob"))
    {
      toggleRoomFlagCommand(RMF_ANY, R_MOBFLAGS);
      return false;
    }
    else if (str.startsWith("_treasure"))
    {
      toggleRoomFlagCommand(RLF_TREASURE, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_armour"))
    {
      toggleRoomFlagCommand(RLF_ARMOUR, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_weapon"))
    {
      toggleRoomFlagCommand(RLF_WEAPON, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_water"))
    {
      toggleRoomFlagCommand(RLF_WATER, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_food"))
    {
      toggleRoomFlagCommand(RLF_FOOD, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_herb"))
    {
      toggleRoomFlagCommand(RLF_HERB, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_key"))
    {
      toggleRoomFlagCommand(RLF_KEY, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_mule"))
    {
      toggleRoomFlagCommand(RLF_MULE, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_horse"))
    {
      toggleRoomFlagCommand(RLF_HORSE, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_pack"))
    {
      toggleRoomFlagCommand(RLF_PACKHORSE, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_trained"))
    {
      toggleRoomFlagCommand(RLF_TRAINEDHORSE, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_rohirrim"))
    {
      toggleRoomFlagCommand(RLF_ROHIRRIM, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_warg"))
    {
      toggleRoomFlagCommand(RLF_WARG, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_boat"))
    {
      toggleRoomFlagCommand(RLF_BOAT, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_attention"))
    {
      toggleRoomFlagCommand(RLF_ATTENTION, R_LOADFLAGS);
      return false;
    }
    else if (str.startsWith("_watch"))
    {
      toggleRoomFlagCommand(RLF_TOWER, R_LOADFLAGS);
      return false;
    }

    if (str.startsWith("_open"))   {dooraction = TRUE; daction = DAT_OPEN;}
    else
      if (str.startsWith("_close"))  {dooraction = TRUE; daction = DAT_CLOSE;}
    else
      if (str.startsWith("_lock"))   {dooraction = TRUE; daction = DAT_LOCK;}
    else
      if (str.startsWith("_unlock")) {dooraction = TRUE; daction = DAT_UNLOCK;}
    else
      if (str.startsWith("_pick"))   {dooraction = TRUE; daction = DAT_PICK;}
    else
      if (str.startsWith("_rock"))   {dooraction = TRUE; daction = DAT_ROCK;}
    else
      if (str.startsWith("_bash"))   {dooraction = TRUE; daction = DAT_BASH;}
    else
      if (str.startsWith("_break"))   {dooraction = TRUE; daction = DAT_BREAK;}
    else
      if (str.startsWith("_block"))   {dooraction = TRUE; daction = DAT_BLOCK;}


    if (str=="_removedoornames") {
      m_mapData->removeDoorNames();
      emit sendToUser("OK. Secret exits purged.\r\n");
    }
    if (str=="_back") {
                        //while (!queue.isEmpty()) queue.dequeue();
      queue.clear();
      emit sendToUser((QByteArray)"OK.\r\n");
      emit showPath(queue, true);
      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false;
    }
    if (str=="_pdynamic") {
      emit sendToUser(m_dynamicRoomDesc.toAscii());
      emit sendToUser((QByteArray)"OK.\r\n");
      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false;
    }
    if (str=="_pstatic") {
      emit sendToUser(m_staticRoomDesc.toAscii());
      emit sendToUser((QByteArray)"OK.\r\n");
      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false;
    }
    if (str=="_help") {
      emit sendToUser((QByteArray)"\r\nMMapper help:\r\n-------------\r\n");

      emit sendToUser((QByteArray)"\r\nStandard MUD commands:\r\n");
      emit sendToUser((QByteArray)"  Move commands: [n,s,...] or [north,south,...]\r\n");
      emit sendToUser((QByteArray)"  Sync commands: [exa,l] or [examine,look]\r\n");

      emit sendToUser((QByteArray)"\r\nManage prespammed command queue:\r\n");
      emit sendToUser((QByteArray)"  _back        - delete prespammed commands from queue\r\n");

      emit sendToUser((QByteArray)"\r\nDescription commands:\r\n");
      emit sendToUser((QByteArray)"  _pdynamic    - prints current room description\r\n");
      emit sendToUser((QByteArray)"  _pstatic     - the same as previous, but without moveable items\r\n");
      emit sendToUser((QByteArray)"  _brief       - emulate brief mode on/off\r\n");

      emit sendToUser((QByteArray)"\r\nHelp commands:\n");
      emit sendToUser((QByteArray)"  _help      - this help text\r\n");
      emit sendToUser((QByteArray)"  _maphelp   - help for mapping console commands\r\n");
      emit sendToUser((QByteArray)"  _doorhelp  - help for door console commands\r\n");
      emit sendToUser((QByteArray)"  _grouphelp - help for group manager console commands\r\n");

      emit sendToUser((QByteArray)"\r\n");

      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false;
    }
    else if (str=="_maphelp")
    {
      emit sendToUser((QByteArray)"\r\nMMapper mapping help:\r\n-------------\r\n");
      emit sendToUser((QByteArray)"\r\nExit commands:\r\n");
      emit sendToUser((QByteArray)"  _name    [n,s,e,w,u,d] [name] - name a door in direction [dir] with [name]\r\n");
      emit sendToUser((QByteArray)"  _door    [n,s,e,w,u,d]        - toggle a door in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _hidden  [n,s,e,w,u,d]        - toggle a hidden door in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _needkey [n,s,e,w,u,d]        - toggle a need key door in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _noblock [n,s,e,w,u,d]        - toggle a no block door in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _nobreak [n,s,e,w,u,d]        - toggle a no break door in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _nopick  [n,s,e,w,u,d]        - toggle a no pick door in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _delayed [n,s,e,w,u,d]        - toggle a delayed door in direction [dir]\r\n");

      emit sendToUser((QByteArray)"\r\n");
      emit sendToUser((QByteArray)"  _exit    [n,s,e,w,u,d]    - toggle a exit in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _road    [n,s,e,w,u,d]    - toggle a road/trail exit in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _climb   [n,s,e,w,u,d]    - toggle a climb exit in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _random  [n,s,e,w,u,d]    - toggle a random exit in direction [dir]\r\n");
      emit sendToUser((QByteArray)"  _special [n,s,e,w,u,d]    - toggle a special exit in direction [dir]\r\n");

      emit sendToUser((QByteArray)"\r\nRoom flag commands:\r\n");
      emit sendToUser((QByteArray)"  _port         - set the room to portable\r\n");
      emit sendToUser((QByteArray)"  _noport       - set the room to not portable\r\n");
      emit sendToUser((QByteArray)"  _dark         - set the room lighting to dark\r\n");
      emit sendToUser((QByteArray)"  _lit          - set the room lighting to lit\r\n");
      emit sendToUser((QByteArray)"  _ride         - set the room to ridable\r\n");
      emit sendToUser((QByteArray)"  _noride       - set the room to not ridable\r\n");
      emit sendToUser((QByteArray)"  _good         - set the room alignment to good\r\n");
      emit sendToUser((QByteArray)"  _neutral      - set the room alignment to neutral\r\n");
      emit sendToUser((QByteArray)"  _evil         - set the room alignment to evil\r\n");

      emit sendToUser((QByteArray)"\r\n");
      emit sendToUser((QByteArray)"  _rent         - toggle a rent mob in the room\r\n");
      emit sendToUser((QByteArray)"  _shop         - toggle a shop in the room\r\n");
      emit sendToUser((QByteArray)"  _weaponshop   - toggle a weapon shop in the room\r\n");
      emit sendToUser((QByteArray)"  _armourshop   - toggle a armour shop in the room\r\n");
      emit sendToUser((QByteArray)"  _foodshop     - toggle a food shop in the room\r\n");
      emit sendToUser((QByteArray)"  _petshop      - toggle a pet shop in the room\r\n");
      emit sendToUser((QByteArray)"  _guild        - toggle a guild in the room\r\n");
      emit sendToUser((QByteArray)"  _scoutguild   - toggle a scout guild in the room\r\n");
      emit sendToUser((QByteArray)"  _mageguild    - toggle a mage guild in the room\r\n");
      emit sendToUser((QByteArray)"  _clericguild  - toggle a cleric guild in the room\r\n");
      emit sendToUser((QByteArray)"  _warriorguild - toggle a warrior guild in the room\r\n");
      emit sendToUser((QByteArray)"  _rangerguild  - toggle a ranger guild in the room\r\n");
      emit sendToUser((QByteArray)"  _smob         - toggle a smob in the room\r\n");
      emit sendToUser((QByteArray)"  _quest        - toggle a quest in the room\r\n");
      emit sendToUser((QByteArray)"  _mob          - toggle a mob in the room\r\n");

      emit sendToUser((QByteArray)"\r\n");
      emit sendToUser((QByteArray)"  _treasure     - toggle a treasure in the room\r\n");
      emit sendToUser((QByteArray)"  _armour       - toggle some armour in the room\r\n");
      emit sendToUser((QByteArray)"  _weapon       - toggle some weapon in the room\r\n");
      emit sendToUser((QByteArray)"  _water        - toggle some water in the room\r\n");
      emit sendToUser((QByteArray)"  _food         - toggle some food in the room\r\n");
      emit sendToUser((QByteArray)"  _herb         - toggle an herb in the room\r\n");
      emit sendToUser((QByteArray)"  _key          - toggle a key in the room\r\n");
      emit sendToUser((QByteArray)"  _mule         - toggle a mule in the room\r\n");
      emit sendToUser((QByteArray)"  _horse        - toggle a horse in the room\r\n");
      emit sendToUser((QByteArray)"  _pack         - toggle a packhorse in the room\r\n");
      emit sendToUser((QByteArray)"  _trained      - toggle a trained horse in the room\r\n");
      emit sendToUser((QByteArray)"  _rohirrim     - toggle a rohirrim horse in the room\r\n");
      emit sendToUser((QByteArray)"  _warg         - toggle a warg in the room\r\n");
      emit sendToUser((QByteArray)"  _boat         - toggle a boat in the room\r\n");
      emit sendToUser((QByteArray)"  _attention    - toggle an attention flag in the room\r\n");
      emit sendToUser((QByteArray)"  _watch        - toggle a watchable spot in the room\r\n"); 

      emit sendToUser((QByteArray)"\r\n");

      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false;
    }
    else if (str=="_doorhelp")
    {
      emit sendToUser((QByteArray)"\r\nMMapper door help:\r\n-------------\r\n");
      emit sendToUser((QByteArray)"\r\nDoor commands:\r\n");
      emit sendToUser((QByteArray)"  _open   [n,s,e,w,u,d]   - open door [dir]\r\n");
      emit sendToUser((QByteArray)"  _close  [n,s,e,w,u,d]   - close door [dir]\r\n");
      emit sendToUser((QByteArray)"  _lock   [n,s,e,w,u,d]   - lock door [dir]\r\n");
      emit sendToUser((QByteArray)"  _unlock [n,s,e,w,u,d]   - unlock door [dir]\r\n");
      emit sendToUser((QByteArray)"  _pick   [n,s,e,w,u,d]   - pick door [dir]\r\n");
      emit sendToUser((QByteArray)"  _rock   [n,s,e,w,u,d]   - throw rock door [dir]\r\n");
      emit sendToUser((QByteArray)"  _bash   [n,s,e,w,u,d]   - bash door [dir]\r\n");
      emit sendToUser((QByteArray)"  _break  [n,s,e,w,u,d]   - cast 'break door' door [dir]\r\n");
      emit sendToUser((QByteArray)"  _block  [n,s,e,w,u,d]   - cast 'block door' door [dir]\r\n");

      emit sendToUser((QByteArray)"\r\nDoor variables:\r\n");
      emit sendToUser((QByteArray)"  $$DOOR_N$$   - secret name of door leading north\r\n");
      emit sendToUser((QByteArray)"  $$DOOR_S$$   - secret name of door leading south\r\n");
      emit sendToUser((QByteArray)"  $$DOOR_E$$   - secret name of door leading east\r\n");
      emit sendToUser((QByteArray)"  $$DOOR_W$$   - secret name of door leading west\r\n");
      emit sendToUser((QByteArray)"  $$DOOR_U$$   - secret name of door leading up\r\n");
      emit sendToUser((QByteArray)"  $$DOOR_D$$   - secret name of door leading down\r\n");
      emit sendToUser((QByteArray)"  $$DOOR$$     - the same as 'exit'\r\n");

      emit sendToUser((QByteArray)"\r\nDestructive commands:\r\n");
      emit sendToUser((QByteArray)"  _removedoornames   - removes all secret door names from the current map\r\n");

      emit sendToUser((QByteArray)"\r\n");

      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false; 
    }
    else if (str=="_grouphelp")
    {
      emit sendToUser((QByteArray)"\r\nMMapper group manager help:\r\n-------------\r\n");
      emit sendToUser((QByteArray)"\r\nGroup commands:\r\n");
      emit sendToUser((QByteArray)"  _gt [message]     - send a grouptell with the [message]\r\n");

      emit sendToUser((QByteArray)"\r\n");

      if (Config().m_mapMode == 2) emit sendToUser("\r\n>");
      return false; 
    }
  }

  if (dooraction) {
    if (str.endsWith(" n"))
      performDoorCommand(NORTH, daction);
    else
      if (str.endsWith(" s"))
        performDoorCommand(SOUTH, daction);
    else
      if (str.endsWith(" e"))
        performDoorCommand(EAST, daction);
    else
      if (str.endsWith(" w"))
        performDoorCommand(WEST, daction);
    else
      if (str.endsWith(" u"))
        performDoorCommand(UP, daction);
    else
      if (str.endsWith(" d"))
        performDoorCommand(DOWN, daction);
    else
      performDoorCommand(UNKNOWN, daction);
    return false;
  }

  if (str=="n" || str=="north"){
    queue.enqueue(CID_NORTH);
    emit showPath(queue, true);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_NORTH);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="s" || str=="south"){
    queue.enqueue(CID_SOUTH);
    emit showPath(queue, true);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_SOUTH);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="e" || str=="east"){
    queue.enqueue(CID_EAST);
    emit showPath(queue, true);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_EAST);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="w" || str=="west"){
    queue.enqueue(CID_WEST);
    emit showPath(queue, true);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_WEST);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="u" || str=="up"){
    queue.enqueue(CID_UP);
    emit showPath(queue, true);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_UP);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="d" || str=="down"){
    queue.enqueue(CID_DOWN);
    emit showPath(queue, true);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_DOWN);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="l" || str=="look"){
    queue.enqueue(CID_LOOK);
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_LOOK);
      return false; //do not send command to mud server for offline mode
    }
  }
  if (str=="exa" || str=="examine"){
    queue.enqueue(CID_LOOK);
    m_examine = true;
    if (Config().m_mapMode != 2)
      return true;
    else
    {
      offlineCharacterMove(CID_LOOK);
      return false; //do not send command to mud server for offline mode
    }
  }
        /*if (str.startsWith("scout")) {
  queue.enqueue(CID_SCOUT);
  if (Config().m_mapMode != 2)
  return true;
}*/
  if ((str=="f" || str=="flee") && Config().m_mapMode == 2){
    offlineCharacterMove(CID_FLEE);
    return false; //do not send command to mud server for offline mode
  }

  if (Config().m_mapMode != 2)
    return true;
  else
  {
    emit sendToUser("\r\n>");
    return false; //do not send command to mud server for offline mode
  }
}

void AbstractParser::offlineCharacterMove(CommandIdType direction)
{
  bool flee = false;
  if (direction == CID_FLEE)
  {
    flee = true;
    emit sendToUser((QByteArray)"You flee head over heels!\r\n");
    direction = (CommandIdType) (rand() % 6);
  }

  if (!flee) queue.dequeue();

  Coordinate c;
  c = m_mapData->getPosition();
  const RoomSelection * rs1 = m_mapData->select(c);
  const Room *rb = rs1->values().front();

  if (direction == CID_LOOK)
  {
    sendRoomInfoToUser(rb);
    sendRoomExitsInfoToUser(rb);
    sendPromptSimulationToUser();
  }
  else
  {
    const Exit &e = rb->exit((uint)direction);
    if ((getFlags(e) & EF_EXIT) && (e.outBegin() != e.outEnd()))
    {
      const RoomSelection * rs2 = m_mapData->select();
      const Room *r = m_mapData->getRoom(*e.outBegin(), rs2);

      sendRoomInfoToUser(r);
      sendRoomExitsInfoToUser(r);
      sendPromptSimulationToUser();
      characterMoved( direction, getName(r), getDynamicDescription(r), getDescription(r), 0, 0);
      emit showPath(queue, true);
      m_mapData->unselect(rs2);
    }
    else
    {
      if (!flee)
        emit sendToUser((QByteArray)"You cannot go that way...");
      else
        emit sendToUser((QByteArray)"You cannot flee!!!");
      emit sendToUser("\r\n>");
    }
  }
  m_mapData->unselect(rs1);
}

void AbstractParser::sendRoomInfoToUser(const Room* r)
{
  if (!r) return;
  emit sendToUser((QByteArray)"\r\n"+getName(r).toAscii()+(QByteArray)"\r\n");
  emit sendToUser(getDescription(r).toAscii().replace("\n","\r\n"));
  emit sendToUser(getDynamicDescription(r).toAscii().replace("\n","\r\n"));
}

void AbstractParser::sendRoomExitsInfoToUser(const Room* r)
{
  if (!r) return;
  QByteArray dn = emptyByteArray;
  QByteArray cn = " -";
  bool noDoors = true;


  QString etmp = "Exits/emulated:";
  int j;
  for (int jj = 0; jj < 7; jj++) {
    switch (jj)
    {
      case 1: j=2;break;
      case 2: j=1;break;
      default: j=jj;break;
    }

    bool door = false;
    bool exit = false;
    bool road = false;
    bool trail = false;
    bool climb = false;
    if (ISSET(getFlags(r->exit(j)),EF_EXIT)) {
      exit = true;
      
      if (ISSET(getFlags(r->exit(j)), EF_ROAD))
	if (getTerrainType(r) == RTT_ROAD) {
	  road = true;
	  etmp += " =";
	}
	else {
	  trail = true;
	  etmp += " -";
	}
      else etmp += " ";
    
      if (ISSET(getFlags(r->exit(j)),EF_DOOR)) {
	door = true;
	etmp += "{";
      } else if (ISSET(getFlags(r->exit(j)),EF_CLIMB)) {
	climb = true;
	etmp += "|";
      }

      switch(j)
      {
        case 0: etmp += "north"; break;
        case 1: etmp += "south"; break;
        case 2: etmp += "east"; break;
        case 3: etmp += "west"; break;
        case 4: etmp += "up"; break;
        case 5: etmp += "down"; break;
        case 6: etmp += "unknown"; break;
      }
    }

    if (door) etmp += "}";
    else if (climb) etmp += "|";
    if (road) etmp += "=";
    else if (trail) etmp += "-";
    if (exit) etmp += ",";
  }
  etmp = etmp.left(etmp.length()-1);
  etmp += ".";

  for (uint i=0;i<6;i++)
  {
    dn = m_mapData->getDoorName(r->getPosition(), i).toAscii();
    if ( dn != emptyByteArray )
    {
      noDoors = false;
      switch (i)
      {
        case 0:
          cn += " n:"+dn;
          break;
        case 1:
          cn += " s:"+dn;
          break;
        case 2:
          cn += " e:"+dn;
          break;
        case 3:
          cn += " w:"+dn;
          break;
        case 4:
          cn += " u:"+dn;
          break;
        case 5:
          cn += " d:"+dn;
          break;
        default:
          break;
      }
    }
  }

  if (noDoors)
  {
    cn = "\r\n";
  }
  else
  {
    cn += ".\r\n";

  }

  emit sendToUser(etmp.toAscii()+cn);
}

void AbstractParser::sendPromptSimulationToUser()
{
  emit sendToUser("\r\n>");
}

void AbstractParser::performDoorCommand(DirectionType direction, DoorActionType action)
{
  QByteArray cn = emptyByteArray;
  QByteArray dn = emptyByteArray;

  switch (action) {
    case DAT_OPEN:
      cn = "open ";
      break;
    case DAT_CLOSE:
      cn = "close ";
      break;
    case DAT_LOCK:
      cn = "lock ";
      break;
    case DAT_UNLOCK:
      cn = "unlock ";
      break;
    case DAT_PICK:
      cn = "pick ";
      break;
    case DAT_ROCK:
      cn = "throw rock ";
      break;
    case DAT_BASH:
      cn = "bash ";
      break;
    case DAT_BREAK:
      cn = "cast 'break door' ";
      break;
    case DAT_BLOCK:
      cn = "cast 'block door' ";
      break;
    default:
      break;
  }

  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  dn = m_mapData->getDoorName(c, direction).toAscii();

  bool needdir = false;

  if (dn == emptyByteArray)
  {
    dn = "exit";
    needdir = true;
  }
  else
    for (int i=0; i<6; i++)
  {
    if ( (((DirectionType)i) != direction) && (m_mapData->getDoorName(c, (DirectionType)i).toAscii() == dn) )
      needdir = true;
  }

  switch (direction)
  {
    case NORTH:
      if (needdir)
        cn += dn+" n\r\n";
      else
        cn += dn+"\r\n";
      break;
    case SOUTH:
      if (needdir)
        cn += dn+" s\r\n";
      else
        cn += dn+"\r\n";
      break;
    case EAST:
      if (needdir)
        cn += dn+" e\r\n";
      else
        cn += dn+"\r\n";
      break;
    case WEST:
      if (needdir)
        cn += dn+" w\r\n";
      else
        cn += dn+"\r\n";
      break;
    case UP:
      if (needdir)
        cn += dn+" u\r\n";
      else
        cn += dn+"\r\n";
      break;
    case DOWN:
      if (needdir)
        cn += dn+" d\r\n";
      else
        cn += dn+"\r\n";
      break;
    default:
      cn += dn+"\r\n";
      break;
  }

  if (Config().m_mapMode != 2)  // online mode
  {
    emit sendToMud(cn);
    emit sendToUser("--->" + cn);
  }
  else
  {
    emit sendToUser("--->" + cn);
    emit sendToUser("OK.\r\n");
  }
}

void AbstractParser::genericDoorCommand(QString command, DirectionType direction)
{
  QByteArray cn = emptyByteArray;
  QByteArray dn = emptyByteArray;

  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  dn = m_mapData->getDoorName(c, direction).toAscii();

  bool needdir = false;

  if (dn == emptyByteArray)
  {
    dn = "exit";
    needdir = true;
  }
  else
    for (int i=0; i<6; i++)
  {
    if ( (((DirectionType)i) != direction) && (m_mapData->getDoorName(c, (DirectionType)i).toAscii() == dn) )
      needdir = true;
  }

  switch (direction)
  {
    case NORTH:
      if (needdir)
        cn += dn+" n\r\n";
      else
        cn += dn+"\r\n";
      command = command.replace("$$DOOR_N$$",cn);
      break;
    case SOUTH:
      if (needdir)
        cn += dn+" s\r\n";
      else
        cn += dn+"\r\n";
      command = command.replace("$$DOOR_S$$",cn);
      break;
    case EAST:
      if (needdir)
        cn += dn+" e\r\n";
      else
        cn += dn+"\r\n";
      command = command.replace("$$DOOR_E$$",cn);
      break;
    case WEST:
      if (needdir)
        cn += dn+" w\r\n";
      else
        cn += dn+"\r\n";
      command = command.replace("$$DOOR_W$$",cn);
      break;
    case UP:
      if (needdir)
        cn += dn+" u\r\n";
      else
        cn += dn+"\r\n";
      command = command.replace("$$DOOR_U$$",cn);
      break;
    case DOWN:
      if (needdir)
        cn += dn+" d\r\n";
      else
        cn += dn+"\r\n";
      command = command.replace("$$DOOR_D$$",cn);
      break;
    default:
      cn += dn+"\r\n";
      command = cn;
      break;
  }

  if (Config().m_mapMode != 2)  // online mode
  {
    emit sendToMud(command.toAscii());
    emit sendToUser("--->" + command.toAscii());
  }
  else
  {
    emit sendToUser("--->" + command.toAscii());
    emit sendToUser("OK.\r\n");
  }
}

void AbstractParser::nameDoorCommand(QString doorname, DirectionType direction)
{
  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  //if (doorname.isEmpty()) toggleExitFlagCommand(EF_DOOR, direction);
  m_mapData->setDoorName(c, doorname, direction);
  emit sendToUser("--->Doorname set to: " + doorname.toAscii() + "\n\r");
}

void AbstractParser::toggleExitFlagCommand(uint flag, DirectionType direction)
{
  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  m_mapData->toggleExitFlag(c, flag, direction, E_FLAGS);

  QString toggle, flagname;
  if (m_mapData->getExitFlag(c, flag, direction, E_FLAGS))
    toggle = "enabled";
  else
    toggle = "disabled";

  switch(flag)
  {
    case EF_EXIT:
      flagname = "Possible";
      break;
    case EF_DOOR:
      flagname = "Door";
      break;
    case EF_ROAD:
      flagname = "Road";
      break;
    case EF_CLIMB:
      flagname = "Climbable";
      break;
    case EF_RANDOM:
      flagname = "Random";
      break;
    case EF_SPECIAL:
      flagname = "Special";
      break;
    default:
      flagname = "Unknown";
      break;
  }

  emit sendToUser("--->" + flagname.toAscii() + " exit " + toggle.toAscii() + "\n\r");
}

void AbstractParser::toggleDoorFlagCommand(uint flag, DirectionType direction)
{
  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  m_mapData->toggleExitFlag(c, flag, direction, E_DOORFLAGS);

  QString toggle, flagname;
  if (m_mapData->getExitFlag(c, flag, direction, E_DOORFLAGS))
    toggle = "enabled";
  else
    toggle = "disabled";

  switch(flag)
  {
    case DF_HIDDEN:
      flagname = "Hidden";
      break;
    case DF_NEEDKEY:
      flagname = "Need key";
      break;
    case DF_NOBLOCK:
      flagname = "No block";
      break;
    case DF_NOBREAK:
      flagname = "No break";
      break;
    case DF_NOPICK:
      flagname = "No pick";
      break;
    case DF_DELAYED:
      flagname = "Delayed";
      break;
    default:
      flagname = "Unknown";
      break;
  }

  emit sendToUser("--->" + flagname.toAscii() + " door " + toggle.toAscii() + "\n\r");
}

void AbstractParser::setRoomFieldCommand(uint flag, uint field)
{
  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  m_mapData->setRoomField(c, flag, field);

  emit sendToUser("--->Room field set\n\r");
}

void AbstractParser::toggleRoomFlagCommand(uint flag, uint field)
{
  Coordinate c;
  QList<Coordinate> cl = m_mapData->getPath(queue);
  if (!cl.isEmpty())
    c = cl.at(cl.size()-1);
  else
    c = m_mapData->getPosition();

  m_mapData->toggleRoomFlag(c, flag, field);

  QString toggle;
  if (m_mapData->getRoomFlag(c, flag, field))
    toggle = "enabled";
  else
    toggle = "disabled";

  emit sendToUser("--->Room flag " + toggle.toAscii() + "\n\r");
}
QString& AbstractParser::latinToAscii(QString& str) {
    // latin1 to 7-bit Ascii
    // taken from Pandora
  const unsigned char table[] = {
/*192*/   'A',
          'A',
          'A',
          'A',
          'A',
          'A',
          'A',
          'C',
          'E',
          'E',
          'E',
          'E',
          'I',
          'I',
          'I',
          'I',
          'D',
          'N',
          'O',
          'O',
          'O',
          'O',
          'O',
          'x',
          'O',
          'U',
          'U',
          'U',
          'U',
          'Y',
          'b',
          'B',
          'a',
          'a',
          'a',
          'a',
          'a',
          'a',
          'a',
          'c',
          'e',
          'e',
          'e',
          'e',
          'i',
          'i',
          'i',
          'i',
          'o',
          'n',
          'o',
          'o',
          'o',
          'o',
          'o',
          ':',
          'o',
          'u',
          'u',
          'u',
          'u',
          'y',
          'b',
          'y'
  };
  unsigned char ch;
  int pos;

  for (pos = 0; pos < str.length(); pos++) {
    ch = str.at(pos).toLatin1();
    if (ch > 128) {
      if (ch < 192)
        ch = 'z';
      else
        ch = table[ ch - 192 ];
      str[pos] = ch;
    }
  }
  return str;
}

