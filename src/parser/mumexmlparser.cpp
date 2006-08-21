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


#include "mumexmlparser.h"
#include "telnetfilter.h"
#include "configuration.h"
#include "patterns.h"
#include "mapdata.h"
#include "mmapper2event.h"
#include "mmapper2room.h"

const QByteArray MumeXmlParser::greatherThanChar(">");
const QByteArray MumeXmlParser::lessThanChar("<");
const QByteArray MumeXmlParser::greatherThanTemplate("&gt;");
const QByteArray MumeXmlParser::lessThanTemplate("&lt;");


MumeXmlParser::MumeXmlParser(MapData* md, QObject *parent) : 
    AbstractParser(md, parent),
	m_readingTag(false),
	m_move(CID_LOOK),
	m_xmlMode(XML_NONE)
//	m_xmlMovement(XMLM_NONE)
{
#ifdef XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE
    QString fileName = "xmlparser_debug.dat";

	file = new QFile(fileName);

    if (!file->open(QFile::WriteOnly))
    	return;

	debugStream = new QDataStream(file);
#endif	
}

MumeXmlParser::~MumeXmlParser()
{
#ifdef XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE
	file->close();
#endif
}


void MumeXmlParser::parseNewMudInput(IncomingData& data)
{
	/*IncomingData data; 	
	//quint8* dline;
	
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
#ifdef XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "OTHER";
		(*debugStream) << "***ETYPE***";
#endif			
				emit sendToUser(data.line);
				break;			

			case TDT_PROMPT:
			case TDT_LF:
			case TDT_LFCR:			
			case TDT_CRLF:										
#ifdef XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***STYPE***";
		(*debugStream) << "CRLF";
		(*debugStream) << "***ETYPE***";
#endif			
				//dline = (quint8 *)data.line.data();
				parse(data.line);
				break;
		}
#ifdef XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE
		(*debugStream) << "***S***";
		(*debugStream) << data.line;
		(*debugStream) << "***E***";
#endif
		
		
	//}
}

void MumeXmlParser::parse(const QByteArray& line)
{
  //quint8* dataline = (quint8*) line.data();
  
  int index;
  for (index = 0; index < line.size(); index++)
  {
 	if (m_readingTag)
 	{
 	  if (line.at(index) == '>')
 	  {
 	  	//send tag
 	  	if (!m_tempTag.isEmpty()) 
 	  		element( m_tempTag );
 	  	
 	  	m_tempTag.clear();
 	  	
 	    m_readingTag = false;
 	    continue;
 	  }
      m_tempTag.append(line.at(index));
 	}
    else
    {
 	  if (line.at(index) == '<')
 	  {
 	  	//send characters
 	  	if (!m_tempCharacters.isEmpty()) 
 	  		characters( m_tempCharacters );
 	  	m_tempCharacters.clear();
 	  	
 	    m_readingTag = true;
 	    continue;
 	  }
      m_tempCharacters.append(line.at(index));
    }  	
  }
  
  if (!m_readingTag)
  {
  	//send characters
  	if (!m_tempCharacters.isEmpty())
  		characters( m_tempCharacters );
  	m_tempCharacters.clear();
  }
}

bool MumeXmlParser::element( const QByteArray& line  )
{
  	//quint8* dataline = (quint8*) line.data();

	int length = line.length();
	switch (m_xmlMode)
	{
		case XML_NONE:
			if (length > 0)
			switch (line.at(0))
			{
				case '/': 
					if (line.startsWith("/xml"))
					{ 
						emit setNormalMode();
						emit sendToUser((QByteArray)"[MMapper] Mode ---> NORMAL\n");
						emptyQueue();
					} 
					break;
				case 'p': 
					if (line.startsWith("prompt")) m_xmlMode = XML_PROMPT; 
					break;						
				case 'e': 
					if (line.startsWith("exits")) m_xmlMode = XML_EXITS; 
					break;
				case 'r': 
					if (line.startsWith("room")) m_xmlMode = XML_ROOM; 
					break;
				case 'm': 
					if (length > 8)
					switch (line.at(8))
					{
						case ' ':
							if (length > 13)
							switch (line.at(13))
							{
								case 'n': 
									m_move = CID_NORTH; 	
									break;
								case 's': 
									m_move = CID_SOUTH; 	
									break;
								case 'e': 
									m_move = CID_EAST; 	
									break;
								case 'w': 
									m_move = CID_WEST; 
									break;
								case 'u': 
									m_move = CID_UP; 		
									break;
								case 'd': 
									m_move = CID_DOWN; 	
									break;
							}
							checkqueue(m_move);
							break;						
						case '/': 
							m_move = CID_NONE; 
							checkqueue(m_move); 
							break;
					}
					break;
			};
						
			/*if (line.startsWith("/xml")){ emit sendToMud((QByteArray)"cha xml\n"); break;}
			if (line.startsWith("prompt")) {m_xmlMode = XML_PROMPT; break;}						
			if (line.startsWith("exits")) {m_xmlMode = XML_EXITS; break;}
			if (line.startsWith("room")) {m_xmlMode = XML_ROOM; break;}
			if (line.startsWith("movement dir=north/")) 	{m_xmlMovement = XMLM_NORTH; checkqueue(); break;}
			if (line.startsWith("movement dir=south/")) 	{m_xmlMovement = XMLM_SOUTH; checkqueue(); break;}
			if (line.startsWith("movement dir=east/")) 	{m_xmlMovement = XMLM_EAST; checkqueue(); break;}
			if (line.startsWith("movement dir=west/")) 	{m_xmlMovement = XMLM_WEST; checkqueue(); break;}
			if (line.startsWith("movement dir=up/")) 		{m_xmlMovement = XMLM_UP; checkqueue(); break;}
			if (line.startsWith("movement dir=down/")) 	{m_xmlMovement = XMLM_DOWN; checkqueue(); break;}
			if (line.startsWith("movement/")) 			{m_xmlMovement = XMLM_UNKNOWN; checkqueue(); break;}
			*/
			break;
		case XML_ROOM:
			if (length > 0)
			switch (line.at(0))
			{
				case 'n': 
					if (line.startsWith("name")) m_xmlMode = XML_NAME; 
					break;
				case 'd': 
					if (line.startsWith("description")) m_xmlMode = XML_DESCRIPTION; 
					break;
				case '/': 
					if (line.startsWith("/room")) m_xmlMode = XML_NONE; 
					break;
			} 
			break;		
		case XML_NAME:
			if (line.startsWith("/name")) m_xmlMode = XML_ROOM;						
			break;
		case XML_DESCRIPTION:
			if (length > 0)
			switch (line.at(0))
			{
				case '/': if (line.startsWith("/description")) m_xmlMode = XML_ROOM; 
					break;
			}
			break;
		case XML_EXITS:
			if (length > 0)
			switch (line.at(0))
			{
				case '/': if (line.startsWith("/exits")) m_xmlMode = XML_NONE;
					break;
			}
			break;
		case XML_PROMPT:
			if (length > 0)
			switch (line.at(0))
			{
				case '/': if (line.startsWith("/prompt")) m_xmlMode = XML_NONE;
					break;
			}
			break;
	}	

  	if (!Config().m_removeXmlTags)
  	{
		emit sendToUser("<"+line+">");
  	}
	return true;
}

void MumeXmlParser::checkqueue(CommandIdType m_xmlMovement)
{
	CommandIdType cid;

	while (!queue.isEmpty())
	{		
		cid = queue.head();

		switch (m_xmlMovement)
		{
			case CID_NORTH:
			case CID_SOUTH:
			case CID_EAST:
			case CID_WEST:
			case CID_UP:
			case CID_DOWN:
				if (cid == CID_FLEE)
				{
					queue.dequeue();
					queue.enqueue(m_xmlMovement);
					return;
				}
					
				if (cid != m_xmlMovement)
					queue.dequeue(); 
				else 
					return;
				if (queue.isEmpty()) 
				{	
					queue.enqueue(m_xmlMovement);
					return;
				}
				break;
			case CID_NONE:
				queue.prepend(CID_NONE);
				//queue.enqueue(CID_NONE);
				//intentional fall through ...
			default:
				return;
				break;
		}
	}	
}

bool MumeXmlParser::characters(QByteArray& ch)
{
 	//quint8* dataline = (quint8*) ch.data();
	
	// replace > and < chars
	ch.replace(greatherThanTemplate, greatherThanChar);
	ch.replace(lessThanTemplate, lessThanChar);

	m_stringBuffer = QString::fromAscii(ch.constData(), ch.size());
	m_stringBuffer = m_stringBuffer.simplified();
	
	switch (m_xmlMode)
	{
		case XML_NONE:	//non room info
			if (m_stringBuffer.isEmpty()) // standard end of description parsed
			{  
				if (m_readingRoomDesc == true)
				{
					m_readingRoomDesc = false; // we finished read desc mode
					m_descriptionReady = true;
					if (Config().m_emulatedExits)
						emulateExits();
				}
			}
			else
			{ 						
				//str=removeAnsiMarks(m_stringBuffer);
				parseMudCommands(m_stringBuffer);
			}
			emit sendToUser(ch);				
			break;

		case XML_ROOM: // dynamic line
			m_dynamicRoomDesc += m_stringBuffer+"\n";
			emit sendToUser(ch);				
			break;

		case XML_NAME:
			if	(m_descriptionReady)
			{			
				m_descriptionReady = false;

				if (Patterns::matchNoDescriptionPatterns(m_roomName)) // non standard end of description parsed (fog, dark or so ...)
				{ 
					m_roomName="";
					m_dynamicRoomDesc="";
					m_staticRoomDesc="";
				} 

				if (!queue.isEmpty())
				{
					CommandIdType c = queue.dequeue();
					if ( c != CID_SCOUT ){
						emit showPath(queue, false);
						characterMoved(c, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
					}
					m_move = CID_LOOK;
				}
				else
				{	
					emit showPath(queue, false);
					characterMoved(m_move, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
					m_move = CID_LOOK;
				}    
			}					
			
			removeAnsiMarks(m_stringBuffer); //Remove room color marks

			m_readingRoomDesc = true; //start of read desc mode
			m_descriptionReady = false;										
			m_roomName=m_stringBuffer;
			m_dynamicRoomDesc="";
			m_staticRoomDesc="";
			m_roomDescLines = 0;
			m_readingStaticDescLines = true;
			m_exitsFlags = 0;

			emit sendToUser(ch);				
			break;

		case XML_DESCRIPTION: // static line
			removeAnsiMarks(m_stringBuffer) ; //remove color marks
			//isStaticRoomDescriptionLine(m_stringBuffer) ; //remove color marks
			m_staticRoomDesc += m_stringBuffer+"\n";
			if (!Config().m_brief) 
				emit sendToUser(ch);				
			break;
		case XML_EXITS:
			parseExits(m_stringBuffer); //parse exits
			if (m_readingRoomDesc)
			{
				m_readingRoomDesc = false;
				m_descriptionReady = true;											
			}
			//emit sendToUser(ch);				
			break;

		case XML_PROMPT:
			if	(m_descriptionReady)
			{			
				m_descriptionReady = false;

				if (Patterns::matchNoDescriptionPatterns(m_roomName)) // non standard end of description parsed (fog, dark or so ...)
				{ 
					m_roomName="";
					m_dynamicRoomDesc="";
					m_staticRoomDesc="";
				} 
				
				parsePrompt(m_stringBuffer);

				if (!queue.isEmpty())
				{
					CommandIdType c = queue.dequeue();
					if ( c != CID_SCOUT ){
						emit showPath(queue, false);
						characterMoved(c, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
					}
					m_move = CID_LOOK;
				}
				else
				{	
					emit showPath(queue, false);
					characterMoved(m_move, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
					m_move = CID_LOOK;
				}    
			}
			else
			{
				if (!queue.isEmpty())
				{
					queue.dequeue();
					emit showPath(queue, true);
				}
			}

			
			ch = ch.trimmed();
			emit sendToUser(ch);				
			break;
	}
	
	return true;
}

void MumeXmlParser::parseMudCommands(QString& str) {

	/*if (str.startsWith('B') && str.startsWith("Brief mode on"))
	{
		emit sendToMud((QByteArray)"brief\n");
		return;
	}*/

	if (str.startsWith('Y'))
	{
		if (str.startsWith("You are dead!"))
		{
			queue.clear();
			emit showPath(queue, true);			
			emit releaseAllPaths();
			return;
		}
		else
		if (str.startsWith("You failed to climb"))
		{
			if(!queue.isEmpty()) queue.dequeue(); 
			queue.prepend(CID_NONE); 
			emit showPath(queue, true);
			return;   
		}
		else		
		if (str.startsWith("You flee head"))
		{
			queue.enqueue(m_move);			
		}
		else
		if (str.startsWith("You follow"))
		{
			queue.enqueue(m_move);			
			return;
		}
	}
}
