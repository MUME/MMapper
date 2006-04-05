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

#ifndef XMLPARSER_H
#define XMLPARSER_H

#include <QXmlDefaultHandler>
#include <QAbstractSocket>
#include "parseevent.h"

class XmlParser : public QXmlDefaultHandler
{
Q_OBJECT
public:
  XmlParser(QAbstractSocket *, QAbstractSocket *);
  bool characters(const QString& ch);
  bool startElement( const QString&, const QString&, const QString& ,
                     const QXmlAttributes& );
  bool endElement( const QString&, const QString&, const QString& );
private:

  void parseExits(const QString & ch);
  void parsePrompt(const QString & ch);
  
  QXmlSimpleReader reader;
  QAbstractSocket * source;
  QAbstractSocket * dest;
  uint expectedData;
  
  CommandIdType c; 
  QString roomName; 
  QString roomDesc; 
  QString parsedRoomDesc; 
  ExitsFlagsType exitFlags;
  PromptFlagsType promptFlags;
  
signals:
  void event(ParseEvent *);
}

#endif
