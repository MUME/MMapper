/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
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

#include "parser.h"
#include "telnetfilter.h"
#include "configuration.h"
#include "patterns.h"
#include "mapdata.h"
#include "mmapper2event.h"
#include "mmapper2room.h"

const QChar Parser::escChar('\x1B');

Parser::Parser(MapData* md, QObject *parent)
: QObject(parent)
{
   m_readingRoomDesc = false;
   m_descriptionReady = false;
   m_following = false;
   m_followDir = UNKNOWN;
   
   m_mapData = md;
   m_roomName = "";
   m_dynamicRoomDesc = "";
   m_roomDescLines = 0;
   m_readingStaticDescLines = true;
   m_staticRoomDesc = "";
   m_exitsFlags = 0;
   m_promptFlags = 0;
   
   m_briefAutoconfigDone = false;
   m_IACPromptAutoconfigDone = false;
   
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
    QString fileName = "parser_debug.dat";

	file = new QFile(fileName);

    if (!file->open(QFile::WriteOnly))
    	return;

	debugStream = new QDataStream(file);
#endif
   
}

Parser::~Parser()
{
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
	file->close();
#endif
}

void Parser::performDoorCommand(DirectionType direction, DoorActionType action)
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

void Parser::genericDoorCommand(QString command, DirectionType direction)
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

void Parser::setDoorCommand(QString doorname, DirectionType direction)
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

void Parser::characterMoved(CommandIdType c, const QString& roomName, const QString& dynamicRoomDesc, const QString& staticRoomDesc, ExitsFlagsType exits, PromptFlagsType prompt)
{
	emit event(createEvent(c, roomName, dynamicRoomDesc, staticRoomDesc, exits, prompt));
}

void Parser::emptyQueue()
{
	//while(!queue.isEmpty()) queue.dequeue();
	queue.clear();
}


QString& Parser::removeAnsiMarks(QString& str) {
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

bool Parser::isRoomName(QString& str)
{
	qint16 index;
	bool ret = false;

	QString col = escChar + Config().m_roomNameColor;
	if ((index=str.indexOf(col)) != -1) {
		
		ret = true;
		index+=col.length();
		
		str.remove(0,index);
		if((index=str.indexOf(escChar)) != -1) {
			str.remove(index,str.length()-index);
		}
	}
	return ret;
}

const char * Parser::q2c(QString & s) {
  return s.toLatin1().constData();
}
 
bool Parser::isStaticRoomDescriptionLine(QString& str) 
{
	qint16 index;
	bool ret = false;

	//QByteArray ba = str.toAscii();
	//quint8* dline = (quint8*) ba.data(); 

	QString col= escChar + Config().m_roomDescColor;
	if ((index=str.indexOf(col)) != -1) {
		ret = true;
		index+=col.length();
		
		str.remove(0,index);

	//ba = str.toAscii();
	//dline = (quint8*) ba.data(); 

		if((index=str.indexOf(escChar)) != -1) {
			str.remove(index,str.length()-index);
		}

	//ba = str.toAscii();
	//dline = (quint8*) ba.data(); 
	}


	return ret;
}

bool Parser::isEndOfRoomDescription(QString& str) {

	if (str.isEmpty()) return true;//false;

	if (Patterns::matchExitsPatterns(str)) {
		m_exitsFlags = EXITS_FLAGS_VALID;

		//const char* data = str.toAscii().data();
		bool doors=FALSE;
		bool road=FALSE;

		for (int i=7; i<str.length(); i++){
			//switch ((int)(data[i])){
			switch ((int)(str.at(i).toAscii())){
			  case 40: doors=true;break;	// (   
			  case 91: doors=true;break;	// [
			  case 61: road=true;break;	   	// =
	
			  case 110:  // n
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

		return true;
	}
	return false;
}

void Parser::parsePrompt(QString& prompt){
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

void Parser::parseMudCommands(QString& str) {

	if (str.startsWith("Brief")){
		if (str.startsWith("Brief mode on")){
			emit sendToMud((QByteArray)"brief\n");
			return;
		}
	
		if (str.startsWith("Brief mode off")){
			return;
		}
	}

	if (str.startsWith("You now follow")){
		m_following = true;   
		emit sendToUser((QByteArray)"----> follow mode on.\n");
		return;
	}

	if (m_following) {
		if (str=="You will not follow anyone else now."){
			m_following = false;   
			emit sendToUser((QByteArray)"----> follow mode off.\n");
			return;
		}

		if (str.contains("leave"))
		{		
			if ((str.contains("leaves north")) || (str.contains("leave north"))) m_followDir = NORTH;
			else
			if ((str.contains("leaves south")) || (str.contains("leave south"))) m_followDir = SOUTH;
			else
			if ((str.contains("leaves east"))  || (str.contains("leave east")))  m_followDir = EAST;
			else
			if ((str.contains("leaves west"))  || (str.contains("leave west")))  m_followDir = WEST;
			else
			if ((str.contains("leaves up"))    || (str.contains("leave up")))    m_followDir = UP;  
			else
			if ((str.contains("leaves down"))  || (str.contains("leave down")))  m_followDir = DOWN;
		}

		if (str.startsWith("You follow"))
		{
			switch (m_followDir) {
			  case NORTH:   queue.enqueue(CID_NORTH);break;     
			  case SOUTH:   queue.enqueue(CID_SOUTH);break;     
			  case EAST:    queue.enqueue(CID_EAST);break;     
			  case WEST:    queue.enqueue(CID_WEST);break;     
			  case UP:      queue.enqueue(CID_UP);break;     
			  case DOWN:    queue.enqueue(CID_DOWN);break;     				  
			  case UNKNOWN: queue.enqueue(CID_NONE);break;     
			}
			return;
		}
	}

	// parse regexps which cancel last char move
	if (Patterns::matchMoveCancelPatterns(str))
	{
		if(!queue.isEmpty()) queue.dequeue(); 
		emit showPath(queue, true);
		return;   
	}

	// parse regexps which force new char move
	if (Patterns::matchMoveForcePatterns(str))
	{
		queue.enqueue(CID_NONE);
		emit showPath(queue, true);
		return;   
	}	
	
	if (str.startsWith("You are dead! Sorry..."))
	{
		queue.clear();
		emit showPath(queue, true);			
		emit releaseAllPaths();
		return;
	}
}

bool Parser::parseUserCommands(QString& command) {
	QString str = command;

	DoorActionType daction;
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

	if (str.startsWith("_"))
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
		queue.enqueue(CID_EXAMINE);
		return true;
	}
	if (str.startsWith("scout")) {
		queue.enqueue(CID_SCOUT);
		return true;
	}

	return true;
}


void Parser::parseNewMudInput(TelnetIncomingDataQueue& que)
{
	IncomingData data; 	
	bool staticLine;
	
	while ( !que.isEmpty() )
	{
		data = que.dequeue();

		staticLine = false;
		
		//dline = (quint8 *)data.line.data();
		switch (data.type)
		{
			case TDT_MENU_PROMPT: 
			case TDT_LOGIN: 
			case TDT_LOGIN_PASSWORD: 
			case TDT_TELNET:
			case TDT_SPLIT:
			case TDT_UNKNOWN:			
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "Other";
		(*debugStream) << "***ETYPE***";
#endif
				emit sendToUser(data.line);
				break;			

			case TDT_PROMPT:
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "Prompt";
		(*debugStream) << "***ETYPE***";
#endif
				if (!m_briefAutoconfigDone)
				{  
					m_briefAutoconfigDone = true;
					emit sendToMud((QByteArray)"brief\n");
				}
				
				if (!m_IACPromptAutoconfigDone && Config().m_IAC_prompt_parser)
				{				
					m_IACPromptAutoconfigDone = true;
					//send IAC-GA prompt request
					QByteArray idprompt("~$#EP2\nG\n");
    				emit sendToMud(idprompt); 
				}
				
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
				
				if (m_readingRoomDesc)
				{
					m_readingRoomDesc = false; // we finished read desc mode
					m_descriptionReady = true;
				}
				
				if	(m_descriptionReady)
				{			
					m_descriptionReady = false;

					parsePrompt(m_stringBuffer);

					if (!queue.isEmpty())
					{
						CommandIdType c = queue.dequeue();
						if ( c != CID_SCOUT ){
							//qDebug("%s",m_roomName.toAscii().data());
							//qDebug("%s",m_dynamicRoomDesc.toAscii().data());
							emit showPath(queue, false);
							characterMoved(c, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
						}
					}
					else
					{	
						//qDebug("%s",m_roomName.toAscii().data());
						//qDebug("%s",m_dynamicRoomDesc.toAscii().data());
						emit showPath(queue, false);
						characterMoved(CID_NONE, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
					}    
				}
				
				emit sendToUser(data.line);				
				break;

			case TDT_CRLF:
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "CRLF";
		(*debugStream) << "***ETYPE***";
#endif			
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
								
				if (m_readingRoomDesc) 
				{					
					if (isEndOfRoomDescription(m_stringBuffer)) // standard end of description parsed
					{  
						m_readingRoomDesc = false; // we finished read desc mode
						m_descriptionReady = true;
					} 
					else // reading room description line 
					{ 
						
						switch(Config().m_roomDescriptionsParserType)
						{
						case Configuration::RDPT_COLOR:
							if ((m_readingStaticDescLines == true) && isStaticRoomDescriptionLine(m_stringBuffer))
							{	
								staticLine = true;
								m_staticRoomDesc += m_stringBuffer+"\n";
							}
							else
							{							
								m_readingStaticDescLines = false;
								m_dynamicRoomDesc += m_stringBuffer+"\n";
							}								
							m_roomDescLines++;														
							break;	
							
						case Configuration::RDPT_PARSER:
							if ((m_roomDescLines >= Config().m_minimumStaticLines) && 
								((m_readingStaticDescLines == false) || 
								Patterns::matchDynamicDescriptionPatterns(m_stringBuffer)))
							{
								m_readingStaticDescLines = false;						
								m_dynamicRoomDesc += m_stringBuffer+"\n";
							}
							else
							{
								staticLine = true;
								m_staticRoomDesc += m_stringBuffer+"\n";								
							}
							m_roomDescLines++;							
							break;	
							
						case Configuration::RDPT_LINEBREAK:
							m_dynamicRoomDesc += m_stringBuffer+"\n";
							m_roomDescLines++;														
							break;								
						}						
						
						
					}
				} 
				else 
				if (!m_readingRoomDesc && m_descriptionReady) //read betwen Exits and Prompt (track for example)
				{
					//*****************					
					if ( isRoomName(m_stringBuffer) ) //Room name arrived
					{ 
						
						
						if	(m_descriptionReady)
						{			
							m_descriptionReady = false;
		
							if (!queue.isEmpty())
							{
								CommandIdType c = queue.dequeue();
								if ( c != CID_SCOUT ){
									emit showPath(queue, false);
									characterMoved(c, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
								}
							}
							else
							{	
								emit showPath(queue, false);
								characterMoved(CID_NONE, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
							}    
						}					
						
						
						m_readingRoomDesc = true; //start of read desc mode
						m_descriptionReady = false;										
						m_roomName=m_stringBuffer;
						m_dynamicRoomDesc="";
						m_staticRoomDesc="";
						m_roomDescLines = 0;
						m_readingStaticDescLines = true;
						m_exitsFlags = 0;
					} 
					else
					if (!m_stringBuffer.isEmpty()) parseMudCommands(m_stringBuffer);					
						  
				} 
				else 
				if ( isRoomName(m_stringBuffer) ) //Room name arrived
				{ 
					
					
					if	(m_descriptionReady)
					{			
						m_descriptionReady = false;
	
						if (!queue.isEmpty())
						{
							CommandIdType c = queue.dequeue();
							if ( c != CID_SCOUT ){
								emit showPath(queue, false);
								characterMoved(c, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
							}
						}
						else
						{	
							emit showPath(queue, false);
							characterMoved(CID_NONE, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
						}    
					}					
					
					
					m_readingRoomDesc = true; //start of read desc mode
					m_descriptionReady = false;										
					m_roomName=m_stringBuffer;
					m_dynamicRoomDesc="";
					m_staticRoomDesc="";
					m_roomDescLines = 0;
					m_readingStaticDescLines = true;
					m_exitsFlags = 0;
				} 
				else 
				if (!m_stringBuffer.isEmpty() && Patterns::matchNoDescriptionPatterns(m_stringBuffer)) // non standard end of description parsed (fog, dark or so ...)
				{ 
					m_readingRoomDesc = false; // we finished read desc mode
					m_descriptionReady = true;
					m_roomName="";
					m_dynamicRoomDesc="";
					m_staticRoomDesc="";
					m_roomDescLines = 0;
					m_readingStaticDescLines = false;
					m_exitsFlags = 0;
					m_promptFlags = 0;
				} 
				else // parse standard input (answers) from server
				{ 
					//str=removeAnsiMarks(m_stringBuffer);
					if (!m_stringBuffer.isEmpty()) parseMudCommands(m_stringBuffer);
				}
										
				if (!(staticLine && Config().m_brief)) emit sendToUser(data.line);				
				break;

			case TDT_LFCR: 
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "LFCR";
		(*debugStream) << "***ETYPE***";
#endif			
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();

				if (m_readingRoomDesc && (Config().m_roomDescriptionsParserType == Configuration::RDPT_LINEBREAK) )
				{
					staticLine = true;
					m_staticRoomDesc += m_stringBuffer+"\n";
					m_roomDescLines++;
				}
				if (!(staticLine && Config().m_brief)) emit sendToUser(data.line);
				break;

			case TDT_LF:
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "LF";
		(*debugStream) << "***ETYPE***";
#endif			
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				emit sendToUser(data.line);
				break;			
		}
		
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***S***";
		(*debugStream) << data.line;
		(*debugStream) << "***E***";
#endif
		
	}
		
}

void Parser::parseNewUserInput(TelnetIncomingDataQueue& que)
{
	IncomingData data; 
	
	quint8 *dline;
	
	while ( !que.isEmpty() )
	{
		data = que.dequeue();
		dline = (quint8 *)data.line.data();
		switch (data.type)
		{
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
	}	
}



