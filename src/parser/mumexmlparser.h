/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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


#ifndef MUMEXMLPARSER_H
#define MUMEXMLPARSER_H

#include <QtCore>
#include "abstractparser.h"

//#define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

enum XmlMode            {XML_NONE, XML_ROOM, XML_NAME, XML_DESCRIPTION, XML_EXITS, XML_PROMPT};
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

    static const QByteArray greatherThanChar;
    static const QByteArray lessThanChar;
    static const QByteArray greatherThanTemplate;
    static const QByteArray lessThanTemplate;

    void parseMudCommands(QString& str);

    bool characters( QByteArray& );
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
