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
