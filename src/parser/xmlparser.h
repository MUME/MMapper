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
