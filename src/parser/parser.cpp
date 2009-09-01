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

#include "parser.h"
#include "telnetfilter.h"
#include "configuration.h"
#include "patterns.h"
#include "mapdata.h"
#include "mmapper2event.h"
#include "mmapper2room.h"

Parser::Parser(MapData* md, QObject *parent)
: AbstractParser(md, parent)
{
   m_following = false;
   m_followDir = UNKNOWN;
   
   m_roomDescLines = 0;
   m_readingStaticDescLines = true;
     
   m_miscAutoconfigDone = false;
   m_IACPromptAutoconfigDone = false;
   m_xmlAutoConfigDone = false;
   
   
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

bool Parser::parseUserCommands(QString& command) {
	if (AbstractParser::parseUserCommands(command)) {
		if (command.startsWith("scout")) {
			queue.enqueue(CID_SCOUT);
		}
		return true;
	}
	else return false;
}

void Parser::parseNewMudInput(IncomingData& data /*TelnetIncomingDataQueue& que*/)
{
	bool staticLine = false;
	bool dontSendToUser = false;
	/*IncomingData data; 	
	bool staticLine;
	
	while ( !que.isEmpty() )
	{
		data = que.dequeue();
*/
		
		//dline = (quint8 *)data.line.data();
		switch (data.type)
		{
			case TDT_DELAY:
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
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
				latinToAscii(m_stringBuffer);
				
				if (m_readingRoomDesc)
				{
					m_readingRoomDesc = false; // we finished read desc mode
					m_descriptionReady = true;
					if (m_examine) m_examine = false; // stop showing bypassing brief-mode
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
						//additional scout move needs to be removed when scout was successful
						else queue.dequeue();
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
				if (data.line.contains("null)>")) break;								

				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
				latinToAscii(m_stringBuffer);
								
				if (m_readingRoomDesc) 
				{					
					if (isEndOfRoomDescription(m_stringBuffer)) // standard end of description parsed
					{  
						m_readingRoomDesc = false; // we finished read desc mode
						m_descriptionReady = true;
						dontSendToUser = true;
					}
					else 
					if (m_stringBuffer.isEmpty())  // standard end of description parsed
					{  
						m_readingRoomDesc = false; // we finished read desc mode
						m_descriptionReady = true;
						if (Config().m_emulatedExits)
							emulateExits();
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
								//additional scout move needs to be removed when scout was successful
								else queue.dequeue();
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
						m_dynamicRoomDesc=nullString;
						m_staticRoomDesc=nullString;
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
							//additional scout move needs to be removed when scout was successful
							else queue.dequeue();
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
					m_dynamicRoomDesc=nullString;
					m_staticRoomDesc=nullString;
					m_roomDescLines = 0;
					m_readingStaticDescLines = true;
					m_exitsFlags = 0;
				} 
				else 
				if (!m_stringBuffer.isEmpty() && Patterns::matchNoDescriptionPatterns(m_stringBuffer)) // non standard end of description parsed (fog, dark or so ...)
				{ 
					m_readingRoomDesc = false; // we finished read desc mode
					m_descriptionReady = true;
					m_roomName=nullString;
					m_dynamicRoomDesc=nullString;
					m_staticRoomDesc=nullString;
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
										
				if (!dontSendToUser && !(staticLine && (m_examine || Config().m_brief))) emit sendToUser(data.line);
				break;

			case TDT_LFCR: 
#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "LFCR";
		(*debugStream) << "***ETYPE***";
#endif			
				m_stringBuffer = QString::fromAscii(data.line.constData(), data.line.size());
				m_stringBuffer = m_stringBuffer.simplified();
				latinToAscii(m_stringBuffer);


				if (m_readingRoomDesc && (Config().m_roomDescriptionsParserType == Configuration::RDPT_LINEBREAK) )
				{
					staticLine = true;
					m_staticRoomDesc += m_stringBuffer+"\n";
					m_roomDescLines++;
				}
				if (!(staticLine && (m_examine || Config().m_brief))) emit sendToUser(data.line);
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
		
	//}
		
}

void Parser::parseMudCommands(QString& str) {

	/*if (str.startsWith('B') && str.startsWith("Brief mode on")){
		emit sendToMud((QByteArray)"brief\n");
	}*/
	
	if (str.startsWith('<') && str.startsWith("<xml>")) //we are in xml mode
	{	emit setXmlMode();
		emit sendToUser((QByteArray)"[MMapper] Mode ---> XML\n");
		emptyQueue();
	}

	if (str.startsWith('Y'))
	{
		if (str.startsWith("You are dead!"))
		{
			queue.clear();
			emit showPath(queue, true);			
			emit releaseAllPaths();
			return;
		}
	
		if (str.startsWith("You flee"))
		{
			if (str.contains("north")) queue.enqueue(CID_NORTH);
			else
			if (str.contains("south")) queue.enqueue(CID_SOUTH);
			else
			if (str.contains("east"))  queue.enqueue(CID_EAST);
			else
			if (str.contains("west"))  queue.enqueue(CID_WEST);
			else
			if (str.contains("up"))    queue.enqueue(CID_UP);  
			else
			if (str.contains("down"))  queue.enqueue(CID_DOWN);
			return;
		}

		if (str.startsWith("You now follow")){
			m_following = true;   
			emit sendToUser((QByteArray)"----> follow mode on.\n");
			return;
		}
		else if (str.startsWith("You quietly scout"))
    		{
      			queue.prepend(CID_SCOUT);
      			return;
    		}
		if (m_following) {
			if (str=="You will not follow anyone else now.")
			{
				m_following = false;   
				emit sendToUser((QByteArray)"----> follow mode off.\n");
				return;
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
	}

	if (m_following) {
		if(str.contains("leave"))
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

bool Parser::isStaticRoomDescriptionLine(QString& str) 
{
	qint16 index;
	bool ret = false;

	QString col= escChar + Config().m_roomDescColor;
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

bool Parser::isEndOfRoomDescription(QString& str) {

	if (Patterns::matchExitsPatterns(str)) {
		parseExits(str);
		return true;
	}
	return false;
}

