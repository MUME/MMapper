/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distributione
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/


#include "abstractparser.h"
#include "mmapper2event.h"
#include "mmapper2room.h"
#include "configuration.h"

const QChar AbstractParser::escChar('\x1B');


AbstractParser::AbstractParser(MapData* md, QObject *parent)
: QObject(parent)
{
   m_readingRoomDesc = false;
   m_descriptionReady = false;
   
   m_mapData = md;
   m_roomName = "";
   m_dynamicRoomDesc = "";
   m_staticRoomDesc = "";
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
	QString out="";
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

	switch (sv=(int)((prompt[index]).toAscii()))
	{
	  case 42: index++;m_promptFlags=SUN_ROOM;break; // *  // sunlight
	  case 33: index++;break; // !  // artifical light
	  case 41: index++;break; // )  // moonlight
	  case 111:index++;break; // o  // darkness
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
	m_exitsFlags = EXITS_FLAGS_VALID;

	//const char* data = str.toAscii().data();
	bool doors=FALSE;
	bool road=FALSE;
	int length = str.length();

	for (int i=7; i<length; i++){
		switch ((int)(str.at(i).toAscii())){
		  case 40: doors=true;break;	// (   
		  case 91: doors=true;break;	// [
		  case 61: road=true;break;	   	// =

		  case 110:  // n 
			  if ( (i+2)<length && (str.at(i+2).toAscii()) == 'r') //north
			  {
				  i+=5;
				  if (doors){
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
			  if (doors){
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
			  if (doors){
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
			  if (doors){
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
			  if (doors){
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
			  if (doors){
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
}

void AbstractParser::parseNewUserInput(IncomingData& data)
{
/*	IncomingData data; 
	
	quint8 *dline;
	
	while ( !que.isEmpty() )
	{
		data = que.dequeue();
*/		
		//dline = (quint8 *)data.line.data();
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
				else
					;//emit sendToMud(QByteArray("\r\n"));
				break;
			case TDT_LFCR: 
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
				if (parseUserCommands(m_stringBuffer))
					emit sendToMud(data.line);
				else
					;//emit sendToMud(QByteArray("\r\n"));
				break;
			case TDT_LF:
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
				if ( parseUserCommands(m_stringBuffer))
					emit sendToMud(data.line);
				else
					;//emit sendToMud(QByteArray("\r\n"));
				break;			
		}
	//}	
}


bool AbstractParser::parseUserCommands(QString& command) {
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
		if (str.contains("$$DOOR_U$$")){genericDoorCommand(command, UP); 	return false;}
		else
		if (str.contains("$$DOOR_D$$")){genericDoorCommand(command, DOWN); return false;}
		else
		if (str.contains("$$DOOR$$"))  {genericDoorCommand(command, UNKNOWN); return false;}
	}

	if (str.startsWith('_'))
	{
		if (str.startsWith("_brief")){
			if (Config().m_brief == TRUE){
				Config().m_brief = FALSE;
				emit sendToUser((QByteArray)"[MMapper] brief mode off.\n");
				return false;
			}		
			if (Config().m_brief == FALSE){
				Config().m_brief = TRUE;
				emit sendToUser((QByteArray)"[MMapper] brief mode on.\n");
				return false;
			}
		}


		if (str.startsWith("_setdoor")){
			if (str.section(" ",1,1)=="n") setDoorCommand(str.section(" ",2,2), NORTH);
			if (str.section(" ",1,1)=="s") setDoorCommand(str.section(" ",2,2), SOUTH);
			if (str.section(" ",1,1)=="e") setDoorCommand(str.section(" ",2,2), EAST);
			if (str.section(" ",1,1)=="w") setDoorCommand(str.section(" ",2,2), WEST);
			if (str.section(" ",1,1)=="u") setDoorCommand(str.section(" ",2,2), UP);
			if (str.section(" ",1,1)=="d") setDoorCommand(str.section(" ",2,2), DOWN);
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


		if (str=="_back") {
			//while (!queue.isEmpty()) queue.dequeue();
			queue.clear();
			emit sendToUser((QByteArray)"OK.\n");
			emit showPath(queue, true);			
			return false;
		}
		if (str=="_pdynamic") {
			emit sendToUser(m_dynamicRoomDesc.toAscii());
			emit sendToUser((QByteArray)"OK.\n");
			return false;
		}
		if (str=="_pstatic") {
			emit sendToUser(m_staticRoomDesc.toAscii());
			emit sendToUser((QByteArray)"OK.\n");
			return false;
		}
		if (str=="_help") {
			emit sendToUser((QByteArray)"\nMMapper help:\n-------------\n");
	
			emit sendToUser((QByteArray)"\nStandard MUD commands:\n");
			emit sendToUser((QByteArray)"  Move commands: [n,s,...] or [north,south,...]\n");
			emit sendToUser((QByteArray)"  Sync commands: [exa,l] or [examine,look]\n");
	
			emit sendToUser((QByteArray)"\nManage prespammed command que:\n");
			emit sendToUser((QByteArray)"  _back        - delete prespammed commands from que\n");
	
			emit sendToUser((QByteArray)"\nDescription commands:\n");
			emit sendToUser((QByteArray)"  _pdynamic    - prints current room description\n");
			emit sendToUser((QByteArray)"  _pstatic     - the same as previous, but without moveable items\n");
			emit sendToUser((QByteArray)"  _brief       - emulate brief mode on/off\n");
	
			emit sendToUser((QByteArray)"\nDoor commands:\n");
			emit sendToUser((QByteArray)"  _open   [n,s,e,w,u,d]   - open door [dir]\n");
			emit sendToUser((QByteArray)"  _close  [n,s,e,w,u,d]   - close door [dir]\n");
			emit sendToUser((QByteArray)"  _lock   [n,s,e,w,u,d]   - lock door [dir]\n");
			emit sendToUser((QByteArray)"  _unlock [n,s,e,w,u,d]   - unlock door [dir]\n");
			emit sendToUser((QByteArray)"  _pick   [n,s,e,w,u,d]   - pick door [dir]\n");
			emit sendToUser((QByteArray)"  _rock   [n,s,e,w,u,d]   - throw rock door [dir]\n");
			emit sendToUser((QByteArray)"  _bash   [n,s,e,w,u,d]   - bash door [dir]\n");
			emit sendToUser((QByteArray)"  _break  [n,s,e,w,u,d]   - cast 'break door' door [dir]\n");
			emit sendToUser((QByteArray)"  _block  [n,s,e,w,u,d]   - cast 'block door' door [dir]\n");
	
			emit sendToUser((QByteArray)"\n");
			emit sendToUser((QByteArray)"  _setdoor [n,s,e,w,u,d] doorname   - sets door name in direction [dir]\n");
	
			emit sendToUser((QByteArray)"\nDoor variables:\n");
			emit sendToUser((QByteArray)"  $$DOOR_N$$   - secret name of door leading north\n");
			emit sendToUser((QByteArray)"  $$DOOR_S$$   - secret name of door leading south\n");
			emit sendToUser((QByteArray)"  $$DOOR_E$$   - secret name of door leading east\n");
			emit sendToUser((QByteArray)"  $$DOOR_W$$   - secret name of door leading west\n");
			emit sendToUser((QByteArray)"  $$DOOR_U$$   - secret name of door leading up\n");
			emit sendToUser((QByteArray)"  $$DOOR_D$$   - secret name of door leading down\n");
			emit sendToUser((QByteArray)"  $$DOOR$$     - the same as 'exit'\n");
	
			emit sendToUser((QByteArray)"\nHelp commands:\n");
			emit sendToUser((QByteArray)"  _help   - this help text\n");
	
			emit sendToUser((QByteArray)"\n");
	
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
		return true;
	}
	if (str=="s" || str=="south"){
		queue.enqueue(CID_SOUTH);
		emit showPath(queue, true);
		return true;
	}
	if (str=="e" || str=="east"){
		queue.enqueue(CID_EAST);
		emit showPath(queue, true);
		return true;
	}
	if (str=="w" || str=="west"){
		queue.enqueue(CID_WEST);
		emit showPath(queue, true);
		return true;
	}
	if (str=="u" || str=="up"){
		queue.enqueue(CID_UP);
		emit showPath(queue, true);
		return true;
	}
	if (str=="d" || str=="down"){
		queue.enqueue(CID_DOWN);
		emit showPath(queue, true);
		return true;
	}
	if (str=="l" || str=="look"){
		queue.enqueue(CID_LOOK);
		return true;
	}
	if (str=="exa" || str=="examine"){
		queue.enqueue(CID_LOOK);
		return true;
	}
	if (str.startsWith("scout")) {
		queue.enqueue(CID_SCOUT);
		return true;
	}

	return true;
}

void AbstractParser::performDoorCommand(DirectionType direction, DoorActionType action)
{
    QByteArray cn = "";
    QByteArray dn = "";
	
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
	if (dn == "") 
		dn = "exit";
		
	switch (direction)
	{
		case NORTH:
			cn += dn+" n\r\n";
			break;
		case SOUTH:
			cn += dn+" s\r\n";
			break;
		case EAST:
			cn += dn+" e\r\n";
			break;
		case WEST:
			cn += dn+" w\r\n";
			break;
		case UP:
			cn += dn+" u\r\n";
			break;
		case DOWN:
			cn += dn+" d\r\n";
			break;
		default:
			cn += dn+"\r\n";
			break;		
	}

    emit sendToMud(cn);
    emit sendToUser("--->" + cn);
}

void AbstractParser::genericDoorCommand(QString command, DirectionType direction)
{
	QByteArray cn = "";
    QByteArray dn = "";

	Coordinate c;
	QList<Coordinate> cl = m_mapData->getPath(queue);
	if (!cl.isEmpty())
		c = cl.at(cl.size()-1);
	else
		c = m_mapData->getPosition();

	dn = m_mapData->getDoorName(c, direction).toAscii();
	if (dn == "") 
		dn = "exit";

	switch (direction)
	{
		case NORTH:
			cn += dn+" n\r\n";
			command = command.replace("$$DOOR_N$$",cn);
			break;
		case SOUTH:
			cn += dn+" s\r\n";
			command = command.replace("$$DOOR_S$$",cn);
			break;
		case EAST:
			cn += dn+" e\r\n";
			command = command.replace("$$DOOR_E$$",cn);
			break;
		case WEST:
			cn += dn+" w\r\n";
			command = command.replace("$$DOOR_W$$",cn);
			break;
		case UP:
			cn += dn+" u\r\n";
			command = command.replace("$$DOOR_U$$",cn);
			break;
		case DOWN:
			cn += dn+" d\r\n";
			command = command.replace("$$DOOR_D$$",cn);
			break;
		default:
			cn += dn+"\r\n";
			command = cn;
			break;		
	}

    emit sendToMud(command.toAscii());
    emit sendToUser("--->" + command.toAscii());
}

void AbstractParser::setDoorCommand(QString doorname, DirectionType direction)
{
	Coordinate c;
	QList<Coordinate> cl = m_mapData->getPath(queue);
	if (!cl.isEmpty())
		c = cl.at(cl.size()-1);
	else
		c = m_mapData->getPosition();

	m_mapData->setDoorName(c, direction, doorname);
    emit sendToUser("--->Doorname set to: " + doorname.toAscii() + "\n\r");
}


