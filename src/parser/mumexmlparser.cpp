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

#include "mumexmlparser.h"
#include "telnetfilter.h"
#include "configuration.h"
#include "patterns.h"
#include "mapdata.h"
#include "mmapper2event.h"
#include "mmapper2room.h"
#include "CGroupCommunicator.h"

#include <qregexp.h>

const QByteArray MumeXmlParser::greaterThanChar(">");
const QByteArray MumeXmlParser::lessThanChar("<");
const QByteArray MumeXmlParser::greaterThanTemplate("&gt;");
const QByteArray MumeXmlParser::lessThanTemplate("&lt;");
const QByteArray MumeXmlParser::ampersand("&");
const QByteArray MumeXmlParser::ampersandTemplate("&amp;");


MumeXmlParser::MumeXmlParser(MapData* md, QObject *parent) :
    AbstractParser(md, parent),
    m_roomDescLines(0), m_readingStaticDescLines(false),
                   m_readingTag(false),
                                m_move(CID_LOOK),
                                       m_xmlMode(XML_NONE)
    //  m_xmlMovement(XMLM_NONE)
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

      /* Jahara Apr-30-09
       * Fixed prompt bug, not sure why it occured
       */
      //emit sendToUser(data.line); 

      parse(data.line);
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
  QByteArray lineToUser;
  int index;

  for (index = 0; index < line.size(); index++) {
    if (m_readingTag) {
      if (line.at(index) == '>') {
        //send tag
        if (!m_tempTag.isEmpty()) element( m_tempTag );

        m_tempTag.clear();

        m_readingTag = false;
        continue;
      }
      m_tempTag.append(line.at(index));
	  
    } else {
      if (line.at(index) == '<') {
        lineToUser.append(characters(m_tempCharacters));
        m_tempCharacters.clear();

        m_readingTag = true;
        continue;
      }
      m_tempCharacters.append(line.at(index));
    }
  }

  if (!m_readingTag) {
    lineToUser.append(characters(m_tempCharacters));
    m_tempCharacters.clear();
  }

  emit sendToUser(lineToUser);
}

bool MumeXmlParser::element( const QByteArray& line  )
{
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
            switch (line.at(8)) {
            case ' ':
			  if (length > 13) {
			        int pos = (line.at(13) == '"') ? 14 : 13;
					switch (line.at(13)) {
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
				};
			  }
              break;
            case '/':
              m_move = CID_NONE;
              break;
			};
          break;
      };
      break;

    case XML_ROOM:
      if (length > 0)
        switch (line.at(0))
      {
        case 'n':
          if (line.startsWith("name")) {
            m_xmlMode = XML_NAME;
            m_roomName = emptyString; // might be empty but valid room name
          }
          break;
        case 'd':
          if (line.startsWith("description")) {
            m_xmlMode = XML_DESCRIPTION;
            m_staticRoomDesc = emptyString; // might be empty but valid description
          }
          break;
        case 't': // terrain tag only comes up in blindness or fog
	  if (line.startsWith("terrain")) m_xmlMode = XML_TERRAIN;
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
    case XML_TERRAIN:
      if (length > 0)
        switch (line.at(0))
      {
        case '/': if (line.startsWith("/terrain")) {
	  m_xmlMode = XML_ROOM;
	  m_readingRoomDesc = true;
	}
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


QByteArray MumeXmlParser::characters(QByteArray& ch)
{
  QByteArray toUser;

  if (ch.isEmpty())
      return toUser;

  // replace > and < chars
  ch.replace(greaterThanTemplate, greaterThanChar);
  ch.replace(lessThanTemplate, lessThanChar);
  ch.replace(ampersandTemplate, ampersand);

  m_stringBuffer = ch.simplified();
  latinToAscii(m_stringBuffer);

  switch (m_xmlMode)
  {
    case XML_NONE:        //non room info
      if (m_stringBuffer.isEmpty()) // standard end of description parsed
      {
        if (m_readingRoomDesc) {
          m_readingRoomDesc = false; // we finished read desc mode
          m_descriptionReady = true;
          if (Config().m_emulatedExits) emulateExits();
        }
      }
      else {
      //str=removeAnsiMarks(m_stringBuffer);
        parseMudCommands(m_stringBuffer);
      }
      toUser.append(ch);
      break;

      case XML_ROOM: // dynamic line
        m_dynamicRoomDesc += m_stringBuffer+"\n";
        toUser.append(ch);
        break;

    case XML_NAME:
      if  (m_descriptionReady) {
        move();
      }

      removeAnsiMarks(m_stringBuffer); //Remove room color marks

      m_readingRoomDesc = true; //start of read desc mode
      m_descriptionReady = false;
      m_roomName=m_stringBuffer;
      m_dynamicRoomDesc=nullString;
      m_staticRoomDesc=nullString;
      m_roomDescLines = 0;
      m_readingStaticDescLines = true;
      m_exitsFlags = 0;

      toUser.append(ch);
      break;

      case XML_DESCRIPTION: // static line
        removeAnsiMarks(m_stringBuffer) ; //remove color marks
        m_staticRoomDesc += m_stringBuffer+"\n";
        if (m_examine || !Config().m_brief)
          toUser.append(ch);
        break;
		
    case XML_EXITS:
	  removeAnsiMarks(m_stringBuffer); // remove color marks
      parseExits(m_stringBuffer); //parse exits
      if (m_readingRoomDesc) {
        m_readingRoomDesc = false;
        m_descriptionReady = true;
      }
      break;

    case XML_PROMPT:
      if (m_readingRoomDesc) { // fixes compact mode
          m_readingRoomDesc = false; // we finished read desc mode
          m_descriptionReady = true;
          if (Config().m_emulatedExits) emulateExits();
      }
      if  (m_descriptionReady) {
        m_examine = false; // stop bypassing brief-mode
        removeAnsiMarks(m_stringBuffer); // remove color marks
        parsePrompt(m_stringBuffer);
        move();
      }
      else
      {
        if (!queue.isEmpty())
        {
          queue.dequeue();
          emit showPath(queue, true);
        }
      }

      toUser.append(ch);
      break;

  case XML_TERRAIN:
  default:
    toUser.append(ch);
    break;
    
  }

  return toUser;
}

void MumeXmlParser::move()
{
  m_descriptionReady = false;

  if (m_roomName == emptyString || // blindness
      Patterns::matchNoDescriptionPatterns(m_roomName)) // non standard end of description parsed (fog, dark or so ...)
  {
    m_roomName=nullString;
    m_dynamicRoomDesc=nullString;
    m_staticRoomDesc=nullString;
  }

  if (!queue.isEmpty())
  {
    CommandIdType c = queue.dequeue();
    if ( c != CID_SCOUT )
    {
      emit showPath(queue, false);
      characterMoved(m_move, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
      if (c != m_move)
        queue.clear();
      m_move = CID_LOOK;
    }
  }
  else
  {
    //emit showPath(queue, false);
    characterMoved(m_move, m_roomName, m_dynamicRoomDesc, m_staticRoomDesc, m_exitsFlags, m_promptFlags);
    m_move = CID_LOOK;
  }
}


void MumeXmlParser::parseMudCommands(QString& str)
{

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
    else if (str.startsWith("You failed to climb"))
    {
      if(!queue.isEmpty()) queue.dequeue();
      queue.prepend(CID_NONE);
      emit showPath(queue, true);
      return;
    }
    else if (str.startsWith("You flee head"))
    {
      queue.enqueue(m_move);
    }
    else if (str.startsWith("You follow"))
    {
      queue.enqueue(m_move);
      return;
    }
    else if (str.startsWith("You quietly scout"))
    {
      queue.prepend(CID_SCOUT);
      return;
    }
  }
  else if (Config().m_groupManagerState !=  CGroupCommunicator::Off)
  {
    if (str.startsWith(escChar))
      removeAnsiMarks(str);

    // Taken from Pandora
    QRegExp scoreExp("[0-9]*/* hits, */* mana, and */* moves.");
    scoreExp.setPatternSyntax(QRegExp::Wildcard);
        // 399/529 hits and 121/133 moves.

    QRegExp scoreTrollExp("[0-9]*/* hits and */* moves.");
    scoreTrollExp.setPatternSyntax(QRegExp::Wildcard);

    // inform groupManager
    if (scoreExp.exactMatch(str) || scoreTrollExp.exactMatch(str))
      emit sendScoreLineEvent(str.toAscii());
  }
}

