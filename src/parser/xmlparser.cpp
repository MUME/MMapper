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

XmlParser::XmlParser(QAbstractSocket * mud, QAbstractSocket * user) : source(mud), dest(user), expectedData(UINT_MAX)
{
  reader.setContentHandler(this);
}

XmlParser::start()
{
  reader.parse(source, true);
  while (1)
  {
    if (source->waitForReadyRead(-1))
      reader.parseContinue();
    else break;
  }
}

bool XmlParser::characters(const QString& ch)
{
  switch (expectedData)
  {
  case EV_NAME: roomName = ch; break;
  case EV_PDESC: parsedRoomDesc = ch; break;
  case EV_EXITS: parseExits(ch); break;
  case EV_PROMPT:
    parsePrompt(ch);
    emit event(createEvent(c, roomName, roomDesc, parsedRoomDesc,  exitFlags, promptFlags));
    roomName = "";
    roomDesc = "";
    parsedRoomDesc = "";
    exitsFlags = 0;
    promptFlags = 0;
    break;
  }
  dest->write(ch);
  return true;
}

bool XmlParser::startElement( const QString & namespaceURI, const QString & localName, const QString & qName, const QXmlAttributes & atts )
{
  if (qName == "name")
    expectedData = EV_NAME;
  else if (qName == "description")
    expectedData = EV_PDESC;
  else if (qName == "exits")
    expectedData = EV_EXITS;
  else if (qName == "prompt")
    expectedData = EV_PROMPT;
  else if (qName == "movement")
  {
    QString dir = atts.value("dir");
    if (dir.isEmpty()) c = CID_NONE;
    else
    {
      switch(dir.toAscii()[0])
      {
      case 'w': c = CID_WEST; break;
      case 'e': c = CID_EAST; break;
      case 'n': c = CID_NORTH; break;
      case 's': c = CID_SOUTH; break;
      case 'd': c = CID_DOWN; break;
      case 'u': c = CID_UP; break;
      }
    }
  }
  return true;
}

bool endElement( const QString&, const QString&, const QString& )
{
  return true;
}
