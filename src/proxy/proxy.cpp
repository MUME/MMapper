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
#include "mumesocket.h"
#include "mpi/mpifilter.h"
#include "mpi/remoteedit.h"
#include "parser/mumexmlparser.h"
#include "expandoracommon/parseevent.h"
#include "pathmachine/mmapper2pathmachine.h"
#include "display/prespammedpath.h"
#include "pandoragroup/mmapper2group.h"
#include "configuration/configuration.h"
#include "clock/mumeclock.h"

#include <qvariant.h>

ProxyThreader::ProxyThreader(Proxy *proxy):
    m_proxy(proxy)
{
}

ProxyThreader::~ProxyThreader()
{
    if (m_proxy) delete m_proxy;
}

void ProxyThreader::run()
{
    try {
        exec();
    } catch (char const *error) {
//          cerr << error << endl;
        throw;
    }
}



Proxy::Proxy(MapData *md, Mmapper2PathMachine *pm, CommandEvaluator *ce, PrespammedPath *pp,
             Mmapper2Group *gm, MumeClock *mc, qintptr &socketDescriptor, bool threaded, QObject *parent)
    : QObject(NULL),
      m_socketDescriptor(socketDescriptor),
      m_mudSocket(NULL),
      m_userSocket(NULL),
      m_serverConnected(false),
      m_telnetFilter(NULL), m_mpiFilter(NULL), m_parserXml(NULL),
      m_mapData(md),
      m_pathMachine(pm),
      m_commandEvaluator(ce),
      m_prespammedPath(pp),
      m_groupManager(gm),
      m_mumeClock(mc),
      m_threaded(threaded),
      m_parent(parent)
{
    if (threaded)
        m_thread = new ProxyThreader(this);
    else
        m_thread = NULL;

    m_remoteEdit = new RemoteEdit(m_parent->parent());

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
        m_userSocket->deleteLater();
    }
    if (m_mudSocket) {
        m_mudSocket->disconnectFromHost();
        m_mudSocket->deleteLater();
    }
    delete m_remoteEdit;
    delete m_telnetFilter;
    delete m_mpiFilter;
    delete m_parserXml;
    connect (this, SIGNAL(doAcceptNewConnections()), m_parent, SLOT(doAcceptNewConnections()));
    emit doAcceptNewConnections();
}

void Proxy::start()
{
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

    connect (this, SIGNAL(log(const QString &, const QString &)), m_parent->parent(),
             SLOT(log(const QString &, const QString &)));

    m_userSocket = new QTcpSocket(this);
    if (!m_userSocket->setSocketDescriptor(m_socketDescriptor)) {
        delete m_userSocket;
        m_userSocket = NULL;
        return false;
    }
    m_userSocket->setSocketOption(QAbstractSocket::KeepAliveOption, true);

    connect(m_userSocket, SIGNAL(disconnected()), this, SLOT(userTerminatedConnection()) );
    connect(m_userSocket, SIGNAL(readyRead()), this, SLOT(processUserStream()) );

    m_telnetFilter = new TelnetFilter(this);
    connect(this, SIGNAL(analyzeUserStream( const QByteArray & )), m_telnetFilter,
            SLOT(analyzeUserStream( const QByteArray & )));
    connect(m_telnetFilter, SIGNAL(sendToMud(const QByteArray &)), this,
            SLOT(sendToMud(const QByteArray &)));
    connect(m_telnetFilter, SIGNAL(sendToUser(const QByteArray &)), this,
            SLOT(sendToUser(const QByteArray &)));

    m_mpiFilter = new MpiFilter(this);
    connect(m_telnetFilter, SIGNAL(parseNewMudInput(IncomingData &)), m_mpiFilter,
            SLOT(analyzeNewMudInput(IncomingData &)));
    connect(m_mpiFilter, SIGNAL(sendToMud(const QByteArray &)), this, SLOT(sendToMud(QByteArray)));
    connect(m_mpiFilter, SIGNAL(editMessage(int, QString, QString)),
            m_remoteEdit, SLOT(remoteEdit(int, QString, QString)), Qt::QueuedConnection);
    connect(m_mpiFilter, SIGNAL(viewMessage(QString, QString)),
            m_remoteEdit, SLOT(remoteView(QString, QString)), Qt::QueuedConnection);
    connect(m_remoteEdit, SIGNAL(sendToSocket(QByteArray)), this, SLOT(sendToMud(QByteArray)),
            Qt::QueuedConnection);

    m_parserXml = new MumeXmlParser(m_mapData, m_mumeClock, this);
    connect(m_mpiFilter, SIGNAL(parseNewMudInput(IncomingData &)), m_parserXml,
            SLOT(parseNewMudInput(IncomingData &)));
    connect(m_telnetFilter, SIGNAL(parseNewUserInput(IncomingData &)), m_parserXml,
            SLOT(parseNewUserInput(IncomingData &)));
    connect(m_parserXml, SIGNAL(sendToMud(const QByteArray &)), this,
            SLOT(sendToMud(const QByteArray &)));
    connect(m_parserXml, SIGNAL(sendToUser(const QByteArray &)), this,
            SLOT(sendToUser(const QByteArray &)));

    connect(m_parserXml, SIGNAL(event(ParseEvent * )), m_pathMachine, SLOT(event(ParseEvent * )),
            Qt::QueuedConnection);
    connect(m_parserXml, SIGNAL(releaseAllPaths()), m_pathMachine, SLOT(releaseAllPaths()),
            Qt::QueuedConnection);
    connect(m_parserXml, SIGNAL(showPath(CommandQueue, bool)), m_prespammedPath,
            SLOT(setPath(CommandQueue, bool)), Qt::QueuedConnection);
    connect(m_parserXml, SIGNAL(log(const QString &, const QString &)), m_parent->parent(),
            SLOT(log(const QString &, const QString &)));
    connect(m_userSocket, SIGNAL(disconnected()), m_parserXml, SLOT(reset()) );

    //Group Manager Support
    connect(m_parserXml, SIGNAL(sendScoreLineEvent(QByteArray)), m_groupManager,
            SLOT(parseScoreInformation(QByteArray)), Qt::QueuedConnection);
    connect(m_parserXml, SIGNAL(sendPromptLineEvent(QByteArray)), m_groupManager,
            SLOT(parsePromptInformation(QByteArray)), Qt::QueuedConnection);
    connect(m_parserXml, SIGNAL(sendGroupTellEvent(QByteArray)), m_groupManager,
            SLOT(sendGTell(QByteArray)), Qt::QueuedConnection);
    // Group Tell
    connect(m_groupManager, SIGNAL(displayGroupTellEvent(const QByteArray &)), m_parserXml,
            SLOT(sendGTellToUser(const QByteArray &)), Qt::QueuedConnection);

    emit log("Proxy", "Connection to client established ...");

    QByteArray ba("\033[1;37;41mWelcome to MMapper!\033[0;37;41m"
                  "   Type \033[1m_help\033[0m\033[37;41m for help or \033[1m_vote\033[0m\033[37;41m to vote!\033[0m\r\n");
    m_userSocket->write(ba);
    m_userSocket->flush();

    m_mudSocket = new MumeMudSocket(this);
    connect(m_mudSocket, SIGNAL(connected()), this, SLOT(onMudConnected()));
    connect(m_mudSocket, SIGNAL(socketError(QAbstractSocket::SocketError)),
            this, SLOT(onMudError(QAbstractSocket::SocketError)));
    connect(m_mudSocket, SIGNAL(disconnected()), this, SLOT(mudTerminatedConnection()) );
    connect(m_mudSocket, SIGNAL(disconnected()), m_parserXml, SLOT(reset()) );
    connect(m_mudSocket, SIGNAL(processMudStream( const QByteArray & )),
            m_telnetFilter, SLOT(analyzeMudStream( const QByteArray & )));
    m_mudSocket->connectToHost();
    return true;
}

void Proxy::onMudConnected()
{
    m_serverConnected = true;

    emit log("Proxy", "Connection to server established ...");

    //send IAC-GA prompt request
    if (Config().m_IAC_prompt_parser) {
        QByteArray idPrompt("~$#EP2\nG\n");
        emit log("Proxy", "Sent MUME Protocol Initiator IAC-GA prompt request");
        emit sendToMud(idPrompt);
    }

    if (Config().m_remoteEditing) {
        QByteArray idRemoteEditing("~$#EI\n");
        emit log("Proxy", "Sent MUME Protocol Initiator remote editing request");
        emit sendToMud(idRemoteEditing);
    }

    emit sendToMud(QByteArray("~$#EX2\n3G\n"));
    emit log("Proxy", "Sent MUME Protocol Initiator XML request");
}

void Proxy::onMudError(QAbstractSocket::SocketError error)
{
    if (error = QAbstractSocket::RemoteHostClosedError) {
        // Handled by mudTerminatedConnection
        return ;
    }

    m_serverConnected = false;

    emit log("Proxy", "MUME is not responding!");

    sendToUser("\r\n"
               "\033[1;37;41mMUME is not responding!\033[0m\r\n"
               "\r\n"
               "\033[1;37;41mYou can explore world map offline or try to reconnect again...\033[0m\r\n"
               "\r\n>");
}

void Proxy::userTerminatedConnection()
{
    emit log("Proxy", "User terminated connection ...");
    m_thread->exit();
}

void Proxy::mudTerminatedConnection()
{
    m_serverConnected = false;

    emit log("Proxy", "Mud terminated connection ...");

    sendToUser("\r\n"
               "\033[1;37;41mMUME closed the connection.\033[0m\r\n"
               "\r\n"
               "\033[1;37;41mYou can explore world map offline or reconnect again...\033[0m\r\n"
               "\r\n>");
}

void Proxy::processUserStream()
{
    int read;
    while (m_userSocket->bytesAvailable()) {
        read = m_userSocket->read(m_buffer, 8191);
        if (read != -1) {
            m_buffer[read] = 0;
            QByteArray ba = QByteArray::fromRawData(m_buffer, read);
            emit analyzeUserStream(ba);
        }
    }
}

void Proxy::sendToMud(const QByteArray &ba)
{
    if (m_mudSocket && m_serverConnected) {
        m_mudSocket->sendToMud(ba);
    }
}

void Proxy::sendToUser(const QByteArray &ba)
{
    if (m_userSocket) {
        m_userSocket->write(ba.data(), ba.size());
        m_userSocket->flush();
    }
}
