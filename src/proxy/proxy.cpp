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

#include "proxy.h"
#include "telnetfilter.h"
#include "parser.h"
#include "mumexmlparser.h"
#include "mainwindow.h"
#include "parseevent.h"
#include "mmapper2pathmachine.h"
#include "prespammedpath.h"
#include "CGroup.h"
#include "configuration.h"

#include <qvariant.h>

ProxyThreader::ProxyThreader(Proxy* proxy):
    m_proxy(proxy)
{
}

ProxyThreader::~ProxyThreader()
{
  if (m_proxy) delete m_proxy;
}

void ProxyThreader::run() {
  try {
    exec();
  } catch (char const * error) {
//          cerr << error << endl;
    throw;
  }
}



Proxy::Proxy(MapData* md, Mmapper2PathMachine* pm, CommandEvaluator* ce, PrespammedPath* pp, CGroup* gm, int & socketDescriptor, QString & host, int & port, bool threaded, QObject *parent)
  : QObject(NULL),
            m_socketDescriptor(socketDescriptor),
                               m_remoteHost(host),
                                            m_remotePort(port),
                                                m_mudSocket(NULL),
                                                    m_userSocket(NULL),
                                                        m_serverConnected(false),
                                                        m_filter(NULL), m_parser(NULL), m_parserXml(NULL),
                                                            m_mapData(md),
                                                                m_pathMachine(pm),
                                                                    m_commandEvaluator(ce),
                                                                        m_prespammedPath(pp),
                                                                            m_groupManager(gm),
                                                                                m_threaded(threaded),
                                                                                m_parent(parent)
{
  if (threaded)
    m_thread = new ProxyThreader(this);
  else
    m_thread = NULL;

#ifdef PROXY_STREAM_DEBUG_INPUT_TO_FILE
  QString fileName = "proxy_debug.dat";

  file = new QFile(fileName);

  if (!file->open(QFile::WriteOnly))
    return;

  debugStream = new QDataStream(file);
#endif
}

Proxy::~Proxy()
{
#ifdef PROXY_STREAM_DEBUG_INPUT_TO_FILE
  file->close();
#endif
  if (m_userSocket) {
    m_userSocket->disconnectFromHost();
    m_userSocket->waitForDisconnected();
    m_userSocket->deleteLater();
  }
  if (m_mudSocket) {
    m_mudSocket->disconnectFromHost();
    m_mudSocket->waitForDisconnected();
    m_mudSocket->deleteLater();
  }
  delete m_filter;
  delete m_parser;
  delete m_parserXml;
  connect (this, SIGNAL(doAcceptNewConnections()), m_parent, SLOT(doAcceptNewConnections()));
  emit doAcceptNewConnections();
}

void Proxy::start() {
  if (m_thread) {
    m_thread->start();
    if (init())
      moveToThread(m_thread);
  }
}


bool Proxy::init()
{
  connect(m_thread, SIGNAL(finished()), this, SLOT(deleteLater()));
  connect (m_thread, SIGNAL(finished()), m_parent, SLOT(doAcceptNewConnections()));

  connect (this, SIGNAL(log(const QString&, const QString&)), m_parent->parent(), SLOT(log(const QString&, const QString&)));

  m_userSocket = new QTcpSocket(this);
  if (!m_userSocket->setSocketDescriptor(m_socketDescriptor))
  {
    emit error(m_userSocket->error());
    delete m_userSocket;
    m_userSocket = NULL;
    return FALSE;
  }

  connect(m_userSocket, SIGNAL(disconnected()), this, SLOT(userTerminatedConnection()) );
  connect(m_userSocket, SIGNAL(readyRead()), this, SLOT(processUserStream()) );

  m_filter = new TelnetFilter(this);
  connect(this, SIGNAL(analyzeUserStream( const char*, int )), m_filter, SLOT(analyzeUserStream( const char*, int )));
  connect(this, SIGNAL(analyzeMudStream( const char*, int )), m_filter, SLOT(analyzeMudStream( const char*, int )));
  connect(m_filter, SIGNAL(sendToMud(const QByteArray&)), this, SLOT(sendToMud(const QByteArray&)));
  connect(m_filter, SIGNAL(sendToUser(const QByteArray&)), this, SLOT(sendToUser(const QByteArray&)));

  m_parser = new Parser(m_mapData, this);
  connect(m_filter, SIGNAL(parseNewMudInput(IncomingData&)), m_parser, SLOT(parseNewMudInput(IncomingData&)));
  connect(m_filter, SIGNAL(parseNewUserInput(IncomingData&)), m_parser, SLOT(parseNewUserInput(IncomingData&)));
  connect(m_parser, SIGNAL(sendToMud(const QByteArray&)), this, SLOT(sendToMud(const QByteArray&)));
  connect(m_parser, SIGNAL(sendToUser(const QByteArray&)), this, SLOT(sendToUser(const QByteArray&)));
  connect(m_parser, SIGNAL(setXmlMode()), m_filter, SLOT(setXmlMode()));

        //connect(m_parser, SIGNAL(event(ParseEvent* )), (QObject*)(Mmapper2PathMachine*)((MainWindow*)(m_parent->parent()))->getPathMachine(), SLOT(event(ParseEvent* )), Qt::QueuedConnection);
  connect(m_parser, SIGNAL(event(ParseEvent* )), m_pathMachine, SLOT(event(ParseEvent* )), Qt::QueuedConnection);
  connect(m_parser, SIGNAL(releaseAllPaths()), m_pathMachine, SLOT(releaseAllPaths()), Qt::QueuedConnection);
  connect(m_parser, SIGNAL(showPath(CommandQueue, bool)), m_prespammedPath, SLOT(setPath(CommandQueue, bool)), Qt::QueuedConnection);

  m_parserXml = new MumeXmlParser(m_mapData, this);
  connect(m_filter, SIGNAL(parseNewMudInputXml(IncomingData&)), m_parserXml, SLOT(parseNewMudInput(IncomingData&)));
  connect(m_filter, SIGNAL(parseNewUserInputXml(IncomingData&)), m_parserXml, SLOT(parseNewUserInput(IncomingData&)));
  connect(m_parserXml, SIGNAL(sendToMud(const QByteArray&)), this, SLOT(sendToMud(const QByteArray&)));
  connect(m_parserXml, SIGNAL(sendToUser(const QByteArray&)), this, SLOT(sendToUser(const QByteArray&)));
  connect(m_parserXml, SIGNAL(setNormalMode()), m_filter, SLOT(setNormalMode()));

        //connect(m_parserXml, SIGNAL(event(ParseEvent* )), (QObject*)(Mmapper2PathMachine*)((MainWindow*)(m_parent->parent()))->getPathMachine(), SLOT(event(ParseEvent* )), Qt::QueuedConnection);
  connect(m_parserXml, SIGNAL(event(ParseEvent* )), m_pathMachine, SLOT(event(ParseEvent* )), Qt::QueuedConnection);
  connect(m_parserXml, SIGNAL(releaseAllPaths()), m_pathMachine, SLOT(releaseAllPaths()), Qt::QueuedConnection);
  connect(m_parserXml, SIGNAL(showPath(CommandQueue, bool)), m_prespammedPath, SLOT(setPath(CommandQueue, bool)), Qt::QueuedConnection);

  //Group Manager Support
  //TODO: Add normal parser support
  connect(m_parserXml, SIGNAL(sendScoreLineEvent(QByteArray)), m_groupManager, SLOT(parseScoreInformation(QByteArray)), Qt::QueuedConnection);
  connect(m_parserXml, SIGNAL(sendPromptLineEvent(QByteArray)), m_groupManager, SLOT(parsePromptInformation(QByteArray)), Qt::QueuedConnection);
  connect(m_parserXml, SIGNAL(sendGroupTellEvent(QByteArray)), m_groupManager, SLOT(sendGTell(QByteArray)), Qt::QueuedConnection);
  // Group Tell
  connect(m_groupManager, SIGNAL(displayGroupTellEvent(const QByteArray&)), this, SLOT(sendToUser(const QByteArray&)), Qt::QueuedConnection);

  //m_userSocket->write("Connection to client established ...\r\n", 38);
  emit log("Proxy", "Connection to client established ...");

  QByteArray ba("\033[1;37;41mWelcome to MMapper!\033[0;37;41m   Type \033[1m_help\033[0m\033[37;41m for help.\033[0m\r\n");
  m_userSocket->write(ba);
  m_userSocket->flush();

  m_mudSocket = new QTcpSocket(this);
  connect(m_mudSocket, SIGNAL(disconnected()), this, SLOT(mudTerminatedConnection()) );
  connect(m_mudSocket, SIGNAL(readyRead()), this, SLOT(processMudStream()) );

  m_mudSocket->connectToHost(m_remoteHost, m_remotePort, QIODevice::ReadWrite);
  if (!m_mudSocket->waitForConnected(5000))
  {
        //m_userSocket->write("Server not responding!!!\r\n", 26);
    emit log("Proxy", "Server not responding!!!");

    sendToUser("\r\nServer not responding!!!\r\n\r\nYou can explore world map offline or try to reconnect again...\r\n");
    sendToUser("\r\n>");
        //m_userSocket->flush();

    emit error(m_mudSocket->error());
    m_mudSocket->close();
    delete m_mudSocket;
    m_mudSocket = NULL;

    return TRUE;

  }
  else
  {
    m_serverConnected = true;
       //m_userSocket->write("Connection to server established ...\r\n", 38);
    emit log("Proxy", "Connection to server established ...");
        //m_userSocket->flush();

    if (Config().m_mpi) {
      emit sendToMud(QByteArray("~$#EX1\n3\n"));
      emit log("Proxy", "Sent MUME Protocol Initiator XML request");
      m_filter->setXmlMode();
    }

    if (Config().m_IAC_prompt_parser) {
      //send IAC-GA prompt request
      QByteArray idprompt("~$#EP2\nG\n");
      emit sendToMud(idprompt);
    }

    return TRUE;
  }
//return TRUE;
  return FALSE;
}



void Proxy::userTerminatedConnection()
{
  emit log("Proxy", "User terminated connection ...");
  m_thread->exit();
}

void Proxy::mudTerminatedConnection()
{
  emit log("Proxy", "Mud terminated connection ...");
  sendToUser("\r\nServer closed the connection\r\n\r\nYou can explore world map offline or try to reconnect again...\r\n");
  sendToUser("\r\n>");
  //m_thread->exit();
}

void Proxy::processUserStream() {
//  emit log("Proxy", "Procesing user input ...");
  int read;
  while(m_userSocket->bytesAvailable()) {
    read = m_userSocket->read(m_buffer, 8191);
    if (read != -1)
    {
      m_buffer[read] = 0;
      //if (m_mudSocket)
      //{
      //    m_mudSocket->write(m_buffer, read);
      //    m_mudSocket->flush();
      //}
      emit analyzeUserStream(m_buffer, read);
    }
  }
}

void Proxy::processMudStream() {
  int read;
  while(m_mudSocket->bytesAvailable()) {
    read = m_mudSocket->read(m_buffer, 8191);
    if (read != -1) {
      m_buffer[read] = 0;
      //if (m_userSocket)
      //{
      //    m_userSocket->write(m_buffer, read);
      //    m_userSocket->flush();
      //}
      emit analyzeMudStream(m_buffer, read);
    }
  }
}


void Proxy::sendToMud(const QByteArray& ba)
{
  if (m_mudSocket && m_serverConnected)
  {
    m_mudSocket->write(ba.data(), ba.size());
    m_mudSocket->flush();

  }
}

void Proxy::sendToUser(const QByteArray& ba)
{
  if (m_userSocket)
  {
    m_userSocket->write(ba.data(), ba.size());
    m_userSocket->flush();

  }
}
