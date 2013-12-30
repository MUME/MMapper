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

#ifndef MUMEXMLPARSER_H
#define MUMEXMLPARSER_H

#include "abstractparser.h"

//#define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

enum XmlMode            {XML_NONE, XML_ROOM, XML_NAME, XML_DESCRIPTION, XML_EXITS, XML_PROMPT, XML_TERRAIN};
//enum XmlMovement    {XMLM_NORTH, XMLM_SOUTH, XMLM_EAST, XMLM_WEST, XMLM_UP, XMLM_DOWN, XMLM_UNKNOWN, XMLM_NONE=10};

class MumeXmlParser : public AbstractParser
{
  Q_OBJECT

  public:

    MumeXmlParser(MapData*, QObject *parent = 0);
    ~MumeXmlParser();


    void parse(const QByteArray& );

  public slots:
    void parseNewMudInput(IncomingData& que);

  protected:

#ifdef XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE
    QDataStream *debugStream;
    QFile* file;
#endif

    static const QByteArray greaterThanChar;
    static const QByteArray lessThanChar;
    static const QByteArray greaterThanTemplate;
    static const QByteArray lessThanTemplate;
    static const QByteArray ampersand;
    static const QByteArray ampersandTemplate;

    void parseMudCommands(QString& str);

    QByteArray characters(QByteArray& ch);
    bool element( const QByteArray& );

    quint32 m_roomDescLines;
    bool m_readingStaticDescLines;

  //void checkqueue(CommandIdType dir = CID_UNKNOWN);
    void move();
    QByteArray m_tempCharacters;
    QByteArray m_tempTag;
    bool m_readingTag;
    CommandIdType m_move;

    XmlMode m_xmlMode;

  signals:
    void sendScoreLineEvent(QByteArray);
};

#endif
