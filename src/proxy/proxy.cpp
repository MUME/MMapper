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

#include <stdexcept>
#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QScopedPointer>
#include <QTcpSocket>

#include "../configuration/configuration.h"
#include "../display/prespammedpath.h"
#include "../global/io.h"
#include "../mpi/mpifilter.h"
#include "../mpi/remoteedit.h"
#include "../pandoragroup/mmapper2group.h"
#include "../parser/abstractparser.h"
#include "../parser/mumexmlparser.h"
#include "../pathmachine/mmapper2pathmachine.h"
#include "connectionlistener.h"
#include "mumesocket.h"
#include "telnetfilter.h"

/* TODO: merge with other use */
#if MMAPPER_NO_OPENSSL
static constexpr const bool NO_OPEN_SSL = true;
#else
static constexpr const bool NO_OPEN_SSL = false;
#endif

class SigParseEvent;

ProxyThreader::ProxyThreader(Proxy *const proxy)
    : m_proxy(proxy)
{}

ProxyThreader::~ProxyThreader()
{
    delete m_proxy;
}

void ProxyThreader::run()
{
    try {
        exec();
    } catch (const std::exception &ex) {
        qCritical() << "Proxy thread is terminating because it threw an exception: " << ex.what()
                    << ".";
        throw;
    } catch (...) {
        qCritical() << "Proxy thread is terminating because it threw an unknown exception.";
        throw;
    }
}

Proxy::Proxy(MapData *const md,
             Mmapper2PathMachine *const pm,
             CommandEvaluator *const ce,
             PrespammedPath *const pp,
             Mmapper2Group *const gm,
             MumeClock *mc,
             qintptr &socketDescriptor,
             const bool threaded,
             ConnectionListener *const listener)
    : QObject(nullptr)
    , m_socketDescriptor(socketDescriptor)
    , m_mapData(md)
    , m_pathMachine(pm)
    , m_commandEvaluator(ce)
    , m_prespammedPath(pp)
    , m_groupManager(gm)
    , m_mumeClock(mc)
    , m_threaded(threaded)
    , m_listener(listener)
{
    if (threaded) {
        m_thread = new ProxyThreader(this);
    } else {
        m_thread = nullptr;
    }

    m_remoteEdit = new RemoteEdit(m_listener->parent());

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
    if (m_userSocket != nullptr) {
        m_userSocket->disconnectFromHost();
        m_userSocket->deleteLater();
    }
    if (m_mudSocket != nullptr) {
        m_mudSocket->disconnectFromHost();
        m_mudSocket->deleteLater();
    }
    delete m_remoteEdit;
    delete m_telnetFilter;
    delete m_mpiFilter;
    delete m_parserXml;
    connect(this, SIGNAL(doAcceptNewConnections()), m_listener, SLOT(doAcceptNewConnections()));
    emit doAcceptNewConnections();
}

void Proxy::start()
{
    if (m_thread != nullptr) {
        m_thread->start();
        if (init()) {
            moveToThread(m_thread);
        }
    } else {
        init();
    }
}

bool Proxy::init()
{
    if (m_threaded) {
        connect(m_thread, &QThread::finished, this, &QObject::deleteLater);
        connect(m_thread,
                &QThread::finished,
                m_listener,
                &ConnectionListener::doAcceptNewConnections);
    }

    connect(this,
            SIGNAL(log(const QString &, const QString &)),
            m_listener->parent(),
            SLOT(log(const QString &, const QString &)));

    m_userSocket.reset(new QTcpSocket(this));
    if (!m_userSocket->setSocketDescriptor(m_socketDescriptor)) {
        m_userSocket.reset(nullptr);
        return false;
    }
    m_userSocket->setSocketOption(QAbstractSocket::KeepAliveOption, true);

    connect(m_userSocket.data(),
            &QAbstractSocket::disconnected,
            this,
            &Proxy::userTerminatedConnection);
    connect(m_userSocket.data(), &QIODevice::readyRead, this, &Proxy::processUserStream);

    m_telnetFilter = new TelnetFilter(this);
    connect(this, &Proxy::analyzeUserStream, m_telnetFilter, &TelnetFilter::analyzeUserStream);
    connect(m_telnetFilter, &TelnetFilter::sendToMud, this, &Proxy::sendToMud);
    connect(m_telnetFilter, &TelnetFilter::sendToUser, this, &Proxy::sendToUser);

    m_mpiFilter = new MpiFilter(this);
    connect(m_telnetFilter,
            &TelnetFilter::parseNewMudInput,
            m_mpiFilter,
            &MpiFilter::analyzeNewMudInput);
    connect(m_mpiFilter, &MpiFilter::sendToMud, this, &Proxy::sendToMud);
    connect(m_mpiFilter, &MpiFilter::editMessage, m_remoteEdit, &RemoteEdit::remoteEdit);
    connect(m_mpiFilter, &MpiFilter::viewMessage, m_remoteEdit, &RemoteEdit::remoteView);
    connect(m_remoteEdit, &RemoteEdit::sendToSocket, this, &Proxy::sendToMud);

    m_parserXml = new MumeXmlParser(m_mapData, m_mumeClock, this);
    connect(m_mpiFilter,
            &MpiFilter::parseNewMudInput,
            m_parserXml,
            &MumeXmlParser::parseNewMudInput);
    connect(m_telnetFilter,
            &TelnetFilter::parseNewUserInput,
            m_parserXml,
            &MumeXmlParser::parseNewUserInput);
    connect(m_parserXml, &MumeXmlParser::sendToMud, this, &Proxy::sendToMud);
    connect(m_parserXml, &MumeXmlParser::sig_sendToUser, this, &Proxy::sendToUser);

    connect(m_parserXml,
            static_cast<void (MumeXmlParser::*)(const SigParseEvent &)>(&MumeXmlParser::event),
            m_pathMachine,
            &Mmapper2PathMachine::event);
    connect(m_parserXml,
            &AbstractParser::releaseAllPaths,
            m_pathMachine,
            &PathMachine::releaseAllPaths);
    connect(m_parserXml, &AbstractParser::showPath, m_prespammedPath, &PrespammedPath::setPath);
    connect(m_parserXml,
            SIGNAL(log(const QString &, const QString &)),
            m_listener->parent(),
            SLOT(log(const QString &, const QString &)));
    connect(m_userSocket.data(),
            &QAbstractSocket::disconnected,
            m_parserXml,
            &AbstractParser::reset);

    //Group Manager Support
    connect(m_parserXml,
            &MumeXmlParser::sendScoreLineEvent,
            m_groupManager,
            &Mmapper2Group::parseScoreInformation);
    connect(m_parserXml,
            &MumeXmlParser::sendPromptLineEvent,
            m_groupManager,
            &Mmapper2Group::parsePromptInformation);
    connect(m_parserXml,
            &AbstractParser::sendGroupTellEvent,
            m_groupManager,
            &Mmapper2Group::sendGTell);
    // Group Tell
    connect(m_groupManager,
            &Mmapper2Group::displayGroupTellEvent,
            m_parserXml,
            &AbstractParser::sendGTellToUser);

    emit log("Proxy", "Connection to client established ...");

    QByteArray ba(
        "\033[1;37;46mWelcome to MMapper!\033[0;37;46m"
        "   Type \033[1m_help\033[0m\033[37;46m for help or \033[1m_vote\033[0m\033[37;46m to vote!\033[0m\r\n");
    m_userSocket->write(ba);
    m_userSocket->flush();

    m_mudSocket = (NO_OPEN_SSL || !Config().connection.tlsEncryption)
                      ? dynamic_cast<MumeSocket *>(new MumeTcpSocket(this))
                      : dynamic_cast<MumeSocket *>(new MumeSslSocket(this));

    connect(m_mudSocket, &MumeSocket::connected, this, &Proxy::onMudConnected);
    connect(m_mudSocket, &MumeSocket::socketError, this, &Proxy::onMudError);
    connect(m_mudSocket, &MumeSocket::disconnected, this, &Proxy::mudTerminatedConnection);
    connect(m_mudSocket, &MumeSocket::disconnected, m_parserXml, &AbstractParser::reset);
    connect(m_mudSocket,
            &MumeSocket::processMudStream,
            m_telnetFilter,
            &TelnetFilter::analyzeMudStream);
    connect(m_mudSocket,
            SIGNAL(log(const QString &, const QString &)),
            m_listener->parent(),
            SLOT(log(const QString &, const QString &)));
    m_mudSocket->connectToHost();
    return true;
}

void Proxy::onMudConnected()
{
    const auto &settings = Config().mumeClientProtocol;

    m_serverConnected = true;

    emit log("Proxy", "Connection to server established ...");

    //send IAC-GA prompt request
    if (settings.IAC_prompt_parser) {
        QByteArray idPrompt("~$#EP2\nG\n");
        emit log("Proxy", "Sent MUME Protocol Initiator IAC-GA prompt request");
        sendToMud(idPrompt);
    }

    if (settings.remoteEditing) {
        QByteArray idRemoteEditing("~$#EI\n");
        emit log("Proxy", "Sent MUME Protocol Initiator remote editing request");
        sendToMud(idRemoteEditing);
    }

    sendToMud(QByteArray("~$#EX2\n3G\n"));
    emit log("Proxy", "Sent MUME Protocol Initiator XML request");
}

void Proxy::onMudError(const QString &errorStr)
{
    m_serverConnected = false;

    qWarning() << "Mud socket error" << errorStr;
    emit log("Proxy", errorStr);

    sendToUser(
        "\r\n"
        "\033[1;37;46m"
        + errorStr.toLocal8Bit()
        + "\033[0m\r\n"
          "\r\n"
          "\033[1;37;46mYou can explore world map offline or try to reconnect again...\033[0m\r\n"
          "\r\n>");
}

void Proxy::userTerminatedConnection()
{
    emit log("Proxy", "User terminated connection ...");
    if (m_threaded) {
        m_thread->exit();
    } else {
        deleteLater();
    }
}

void Proxy::mudTerminatedConnection()
{
    if (!m_serverConnected) {
        return;
    }

    m_serverConnected = false;

    emit log("Proxy", "Mud terminated connection ...");

    sendToUser("\r\n"
               "\033[1;37;46mMUME closed the connection.\033[0m\r\n"
               "\r\n"
               "\033[1;37;46mYou can explore world map offline or reconnect again...\033[0m\r\n"
               "\r\n>");
}

void Proxy::processUserStream()
{
    if (m_userSocket != nullptr) {
        io::readAllAvailable(*m_userSocket, m_buffer, [this](QByteArray byteArray) {
            if (byteArray.size() != 0)
                emit analyzeUserStream(byteArray);
        });
    }
}

void Proxy::sendToMud(const QByteArray &ba)
{
    if (m_mudSocket != nullptr) {
        m_mudSocket->sendToMud(ba);
    } else {
        qWarning() << "Mud socket not available";
    }
}

void Proxy::sendToUser(const QByteArray &ba)
{
    if (m_userSocket != nullptr) {
        m_userSocket->write(ba.data(), ba.size());
        m_userSocket->flush();
    } else {
        qWarning() << "User socket not available";
    }
}
