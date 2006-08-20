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
#include "mmapper2exit.h"
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
	
	Coordinate c;
    QByteArray dn = "";
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
	//emit sendToUser(cn);
}

void AbstractParser::emulateExits()
{
	Coordinate c;
//    QByteArray dn = "";
//    QByteArray cn = "Exits: ";
    
	CommandQueue tmpqueue;
//	bool noDoors = true;
	
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
	//emit sendToUser(cn);	*/
	
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


	// if (Config().m_mapMode == 2)  // offline mode

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
				emit sendToUser((QByteArray)"[MMapper] brief mode off.\r\n");
				return false;
			}		
			if (Config().m_brief == FALSE){
				Config().m_brief = TRUE;
				emit sendToUser((QByteArray)"[MMapper] brief mode on.\r\n");
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
	
			emit sendToUser((QByteArray)"\r\nManage prespammed command que:\r\n");
			emit sendToUser((QByteArray)"  _back        - delete prespammed commands from que\r\n");
	
			emit sendToUser((QByteArray)"\r\nDescription commands:\r\n");
			emit sendToUser((QByteArray)"  _pdynamic    - prints current room description\r\n");
			emit sendToUser((QByteArray)"  _pstatic     - the same as previous, but without moveable items\r\n");
			emit sendToUser((QByteArray)"  _brief       - emulate brief mode on/off\r\n");
	
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
	
			emit sendToUser((QByteArray)"\r\n");
			emit sendToUser((QByteArray)"  _setdoor [n,s,e,w,u,d] doorname   - sets door name in direction [dir]\r\n");
	
			emit sendToUser((QByteArray)"\r\nDoor variables:\r\n");
			emit sendToUser((QByteArray)"  $$DOOR_N$$   - secret name of door leading north\r\n");
			emit sendToUser((QByteArray)"  $$DOOR_S$$   - secret name of door leading south\r\n");
			emit sendToUser((QByteArray)"  $$DOOR_E$$   - secret name of door leading east\r\n");
			emit sendToUser((QByteArray)"  $$DOOR_W$$   - secret name of door leading west\r\n");
			emit sendToUser((QByteArray)"  $$DOOR_U$$   - secret name of door leading up\r\n");
			emit sendToUser((QByteArray)"  $$DOOR_D$$   - secret name of door leading down\r\n");
			emit sendToUser((QByteArray)"  $$DOOR$$     - the same as 'exit'\r\n");
	
			emit sendToUser((QByteArray)"\r\nHelp commands:\n");
			emit sendToUser((QByteArray)"  _help   - this help text\r\n");
	
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
		if (Config().m_mapMode != 2) 
			return true; 
		else 
		{
			offlineCharacterMove(CID_LOOK);
			return false; //do not send command to mud server for offline mode
		}
	}
	if (str.startsWith("scout")) {
		queue.enqueue(CID_SCOUT);
		if (Config().m_mapMode != 2) 
			return true; 
	}
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
		emit sendToUser((QByteArray)"You fleed over heels!\r\n");
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
			sendRoomExitsInfoToUser(rb);
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
    QByteArray dn = "";
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
    	if (ISSET(getFlags(r->exit(j)),EF_DOOR)) 
    	{
    		door = true;
    		etmp += " {";	
    	}
    		
        if (ISSET(getFlags(r->exit(j)),EF_EXIT)) {
        	exit = true;
    		if (!door) etmp += " ";
    		
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
        
        if (door)
        {
        	/*if (getDoorName(r->exit(j))!="")
        		etmp += "/"+getDoorName(r->exit(j))+"}";	
        	else*/
        		etmp += "},";				                		
        }
        else
        {
        	if (exit)
      			etmp += ",";				                		
        }
    }
    etmp = etmp.left(etmp.length()-1);
	etmp += ".";							

	for (uint i=0;i<6;i++)
	{
	  dn = m_mapData->getDoorName(r->getPosition(), i).toAscii();
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
	
	emit sendToUser(etmp.toAscii()+cn);
	
}

void AbstractParser::sendPromptSimulationToUser()
{
	emit sendToUser("\r\n>");
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
		
	bool needdir = false;

	if (dn == "") 
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
	QByteArray cn = "";
    QByteArray dn = "";

	Coordinate c;
	QList<Coordinate> cl = m_mapData->getPath(queue);
	if (!cl.isEmpty())
		c = cl.at(cl.size()-1);
	else
		c = m_mapData->getPosition();

	dn = m_mapData->getDoorName(c, direction).toAscii();

	bool needdir = false;

	if (dn == "") 
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


