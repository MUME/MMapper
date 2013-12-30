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

#ifndef PARSER_H
#define PARSER_H

//#define PARSER_STREAM_DEBUG_INPUT_TO_FILE

#include "abstractparser.h"

class Parser : public AbstractParser
{
Q_OBJECT
public:
    Parser(MapData*, QObject *parent = 0);
    ~Parser();
    
public slots:
  	void parseNewMudInput(IncomingData&);
    
protected:

#ifdef PARSER_STREAM_DEBUG_INPUT_TO_FILE
	QDataStream *debugStream;
	QFile* file;
#endif

   bool isRoomName(QString& str);
   bool isEndOfRoomDescription(QString& str);	
   bool isStaticRoomDescriptionLine(QString& str);
   virtual bool parseUserCommands(QString& command);

   void parseMudCommands(QString& str);

   quint32 m_roomDescLines;
   bool m_readingStaticDescLines;
   
   bool m_following;
   quint16 m_followDir;
   
   bool m_miscAutoconfigDone;
   bool m_IACPromptAutoconfigDone;
   bool m_xmlAutoConfigDone;
};

#endif
