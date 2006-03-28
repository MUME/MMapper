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

#include "proxy.h"
#include "telnetfilter.h"
#include "parser.h"
#include "mainwindow.h"
#include "parseevent.h"
#include "mmapper2pathmachine.h"
#include "prespammedpath.h" 

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
//	    cerr << error << endl;
	    throw error;
  }
}



Proxy::Proxy(MapData* md, Mmapper2PathMachine* pm, CommandEvaluator* ce, PrespammedPath* pp, int & socketDescriptor, QString & host, int & port, bool threaded, QObject *parent)
    : QObject(NULL),
    m_socketDescriptor(socketDescriptor),
    m_remoteHost(host),
    m_remotePort(port),
    m_mudSocket(NULL),
    m_userSocket(NULL),
    m_mapData(md),    
    m_pathMachine(pm),
    m_commandEvaluator(ce),
    m_prespammedPath(pp),
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
    m_userSocket->disconnectFromHost();
    m_userSocket->waitForDisconnected();
    delete m_filter;
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

	m_parser = new Parser(m_mapData, this);
    connect(m_filter, SIGNAL(parseNewMudInput(TelnetIncomingDataQueue&)), m_parser, SLOT(parseNewMudInput(TelnetIncomingDataQueue&)));
    connect(m_filter, SIGNAL(parseNewUserInput(TelnetIncomingDataQueue&)), m_parser, SLOT(parseNewUserInput(TelnetIncomingDataQueue&)));
    connect(m_parser, SIGNAL(sendToMud(const QByteArray&)), this, SLOT(sendToMud(const QByteArray&)));
    connect(m_parser, SIGNAL(sendToUser(const QByteArray&)), this, SLOT(sendToUser(const QByteArray&)));
	
	//connect(m_parser, SIGNAL(event(ParseEvent* )), (QObject*)(Mmapper2PathMachine*)((MainWindow*)(m_parent->parent()))->getPathMachine(), SLOT(event(ParseEvent* )), Qt::QueuedConnection);
	connect(m_parser, SIGNAL(event(ParseEvent* )), m_pathMachine, SLOT(event(ParseEvent* )), Qt::QueuedConnection);
	connect(m_parser, SIGNAL(releaseAllPaths()), m_pathMachine, SLOT(releaseAllPaths()), Qt::QueuedConnection);

	connect(m_parser, SIGNAL(showPath(CommandQueue, bool)), m_prespammedPath, SLOT(setPath(CommandQueue, bool)), Qt::QueuedConnection);

    //m_userSocket->write("Connection to client established ...\r\n", 38);
    emit log("Proxy", "Connection to client established ...");
    m_userSocket->flush();

    m_mudSocket = new QTcpSocket(this);
    connect(m_mudSocket, SIGNAL(disconnected()), this, SLOT(mudTerminatedConnection()) );
    connect(m_mudSocket, SIGNAL(readyRead()), this, SLOT(processMudStream()) );

    m_mudSocket->connectToHost(m_remoteHost, m_remotePort, QIODevice::ReadWrite);
    if (!m_mudSocket->waitForConnected(5000))
    {
        //m_userSocket->write("Server not responding!!!\r\n", 26);
        emit log("Proxy", "Server not responding!!!");
        //m_userSocket->flush();

        emit error(m_mudSocket->error());

    }
    else
    {
        //m_userSocket->write("Connection to server established ...\r\n", 38);
        emit log("Proxy", "Connection to server established ...");
        //m_userSocket->flush();

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
    m_thread->exit();
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
  if (m_mudSocket) 
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
