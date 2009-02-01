/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef PARSER_H
#define PARSER_H

//#define PARSER_STREAM_DEBUG_INPUT_TO_FILE

#include <QtGui>
#include <QtCore>
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
   
   QString m_stringBuffer;
   QByteArray m_byteBuffer;
   
   bool m_miscAutoconfigDone;
   bool m_IACPromptAutoconfigDone;
   bool m_xmlAutoConfigDone;
 
};



#endif
