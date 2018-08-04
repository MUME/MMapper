/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
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

#include "CGroupCommunicator.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QHash>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

#include "../configuration/configuration.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupClient.h"
#include "CGroupServer.h"
#include "groupaction.h"
#include "groupselection.h"
#include "mmapper2group.h"

using Messages = CGroupCommunicator::Messages;

CGroupCommunicator::CGroupCommunicator(GroupManagerState type, QObject *parent)
    : QObject(parent)
    , type(type)
{
    connect(this, SIGNAL(sendLog(const QString &)), parent, SLOT(sendLog(const QString &)));
    connect(this, SIGNAL(messageBox(QString)), parent, SLOT(relayMessageBox(QString)));
    connect(this, SIGNAL(gTellArrived(QDomNode)), parent, SLOT(gTellArrived(QDomNode)));
    connect(this, SIGNAL(networkDown()), parent, SLOT(networkDown()));
    connect(this,
            &CGroupCommunicator::scheduleAction,
            (dynamic_cast<Mmapper2Group *>(parent))->getGroup(),
            &CGroup::scheduleAction);
}

//
// Communication protocol switches and logic
//

//
// Low level. Message forming and messaging
//
QByteArray CGroupCommunicator::formMessageBlock(Messages message, const QDomNode &data)
{
    QByteArray block;

    QDomDocument doc("datagram");
    QDomElement root = doc.createElement("datagram");
    root.setAttribute("message", static_cast<int>(message));
    doc.appendChild(root);

    QDomElement dataElem = doc.createElement("data");
    dataElem.appendChild(doc.importNode(data, true));
    root.appendChild(dataElem);

    block = doc.toString().toLatin1();
    //NOTE: VERY Spammy
    //    qInfo("Message: %s", (const char *) block);

    return block;
}

void CGroupCommunicator::sendMessage(CGroupClient *connection,
                                     Messages message,
                                     const QByteArray &blob)
{
    QDomDocument doc("datagram");
    QDomElement root = doc.createElement("text");

    QDomText t = doc.createTextNode("dataText");
    t.setNodeValue(blob);
    root.appendChild(t);

    sendMessage(connection, message, root);
}

void CGroupCommunicator::sendMessage(CGroupClient *connection,
                                     Messages message,
                                     const QDomNode &node)
{
    connection->sendData(formMessageBlock(message, node));
}

// the core of the protocol
void CGroupCommunicator::incomingData(CGroupClient *conn, const QByteArray &buff)
{
    QString data;

    data = buff;

    // NOTE: Very spammy
    //    qInfo("Raw data received: %s", (const char *) data.toLatin1());

    QString error;
    int errLine, errColumn;
    QDomDocument doc("datagram");
    if (!doc.setContent(data, &error, &errLine, &errColumn)) {
        qWarning("Failed to parse the datagram. Error in line %i, column %i: %s",
                 errLine,
                 errColumn,
                 error.toLatin1().constData());
        return;
    }

    QDomNode n = doc.documentElement();
    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (e.nodeName() == "datagram") {
            QDomElement dataElement = e.firstChildElement();
            // TODO: need stronger type checking
            Messages message = static_cast<Messages>(e.attribute("message").toInt());

            // converting a given node to the text form.
            retrieveData(conn, message, dataElement);
        }
        n = n.nextSibling();
    }
}

// this function is for sending gtell from a local user
void CGroupCommunicator::sendGTell(const QByteArray &tell)
{
    // form the gtell QDomNode first.
    QDomDocument doc("datagram");
    QDomElement root = doc.createElement("gtell");
    root.setAttribute("from", QString(Config().groupManager.charName));

    QDomText t = doc.createTextNode("dataText");
    t.setNodeValue(tell);
    root.appendChild(t);

    // depending on the type of this communicator either send to
    // server or send to everyone
    sendGroupTellMessage(root);
}

void CGroupCommunicator::sendCharUpdate(CGroupClient *connection, const QDomNode &blob)
{
    sendMessage(connection, Messages::UPDATE_CHAR, blob);
}

void CGroupCommunicator::renameConnection(const QByteArray &oldName, const QByteArray &newName)
{
    //    qInfo() << "Renaming from" << oldName << "to" << newName;

    // form the rename QDomNode first.
    QDomDocument doc("datagram");
    QDomElement root = doc.createElement("rename");
    root.setAttribute("oldname", QString(oldName));
    root.setAttribute("newname", QString(newName));

    sendCharRename(root);
}

//
// ******************** S E R V E R   S I D E ******************
//
// Server side of the communication protocol
CGroupServerCommunicator::CGroupServerCommunicator(QObject *parent)
    : CGroupCommunicator(GroupManagerState::Server, parent)
{
    emit sendLog("Server mode has been selected");
    reconnect();
}

CGroupServerCommunicator::~CGroupServerCommunicator()
{
    if (server != nullptr) {
        server->closeAll();
        delete server;
        server = nullptr;
    }
}

void CGroupServerCommunicator::connectionClosed(CGroupClient *connection)
{
    auto sock = connection->socketDescriptor();

    QByteArray name = clientsList.key(sock);
    if (name != "") {
        sendRemoveUserNotification(connection, name);
        clientsList.remove(name);
        emit sendLog(QString("'%1' closed their connection and quit.").arg(QString(name)));
        emit scheduleAction(new RemoveCharacter(name));
    }
}

void CGroupServerCommunicator::serverStartupFailed()
{
    emit messageBox(
        QString("Failed to start the groupManager server: %1.").arg(server->errorString()));
    disconnect();
    emit networkDown();
}

void CGroupServerCommunicator::connectionEstablished(CGroupClient *connection)
{
    //    qInfo() << "Detected new connection, starting handshake";
    sendMessage(connection, Messages::REQ_LOGIN, "test");
}

void CGroupServerCommunicator::retrieveData(CGroupClient *connection,
                                            Messages message,
                                            QDomNode data)
{
    //    qInfo() << "Retrieve data from" << conn;
    switch (connection->getConnectionState()) {
    //Closed, Connecting, Connected, Quiting
    case ConnectionStates::Connected:
        // AwaitingLogin, AwaitingInfo, Logged

        // ---------------------- AwaitingLogin  --------------------
        if (connection->getProtocolState() == ProtocolStates::AwaitingLogin) {
            // Login state. either REQ_LOGIN or ACK should come
            if (message == Messages::UPDATE_CHAR) {
                // aha! parse the data
                connection->setProtocolState(ProtocolStates::AwaitingInfo);
                parseLoginInformation(connection, data);
            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
            }
            // ---------------------- AwaitingInfo  --------------------
        } else if (connection->getProtocolState() == ProtocolStates::AwaitingInfo) {
            // almost connected. awaiting full information about the connection
            if (message == Messages::REQ_INFO) {
                sendGroupInformation(connection);
                sendMessage(connection, Messages::REQ_ACK);
            } else if (message == Messages::ACK) {
                connection->setProtocolState(ProtocolStates::Logged);
                sendMessage(connection, Messages::STATE_LOGGED);
            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
            }

            // ---------------------- LOGGED --------------------
        } else if (connection->getProtocolState() == ProtocolStates::Logged) {
            // usual update situation. receive update, unpack, apply.
            if (message == Messages::UPDATE_CHAR) {
                emit scheduleAction(new UpdateCharacter(data.firstChildElement()));
                relayMessage(connection, Messages::UPDATE_CHAR, data.firstChildElement());
            } else if (message == Messages::GTELL) {
                emit gTellArrived(data);
                relayMessage(connection, Messages::GTELL, data.firstChildElement());
            } else if (message == Messages::REQ_ACK) {
                sendMessage(connection, Messages::ACK);
            } else if (message == Messages::RENAME_CHAR) {
                emit scheduleAction(new RenameCharacter(data.firstChildElement()));
                relayMessage(connection, Messages::RENAME_CHAR, data.firstChildElement());
            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(Logged) Unexpected message marker. Trying to ignore.");
            }
        }

        break;

    case ConnectionStates::Closed:

    case ConnectionStates::Connecting:
    case ConnectionStates::Quiting:
        qWarning("Data arrival during wrong connection state.");
        break;
    }
}

void CGroupServerCommunicator::sendCharUpdate(QDomNode blob)
{
    if (Config().groupManager.shareSelf) {
        QByteArray message = formMessageBlock(Messages::UPDATE_CHAR, blob);
        server->sendToAll(message);
    }
}

void CGroupServerCommunicator::parseLoginInformation(CGroupClient *connection, const QDomNode &data)
{
    //    qInfo() << "Parsing login information from" << conn->socketDescriptor();

    if (data.nodeName() != "data") {
        qWarning("Called parseLoginInformation with wrong node. No data node.");
        return;
    }

    QDomElement node = data.firstChildElement().toElement();

    if (node.nodeName() != "loginData") {
        qWarning("Called parseLoginInformation with wrong node. No loginData node.");
        return;
    }

    int ver = node.attribute("protocolVersion").toInt();

    QByteArray kickMessage = "";
    if (ver < protocolVersion) {
        kickMessage = "Server uses newer version of the protocol. Please update.";
    }

    if (ver > protocolVersion) {
        kickMessage = "Server uses older version of the protocol.";
    }

    QDomNode charNode = node.firstChildElement();
    if (charNode.nodeName() != "playerData") {
        qWarning("Called parseLoginInformation with wrong node. No playerData node.");
        return;
    }

    QByteArray name = charNode.toElement().attribute("name").toLatin1();
    emit sendLog(QString("'%1's protocol version: %2").arg(name.constData()).arg(ver));
    if (getGroup()->isNamePresent(name)) {
        kickMessage = "The name you picked is already present!";
    } else {
        clientsList.insert(name, connection->socketDescriptor());
        emit scheduleAction(new AddCharacter(charNode));
        relayMessage(connection, Messages::ADD_CHAR, charNode);
    }

    if (kickMessage != "") {
        // kicked
        //        qInfo() << "Kicking conn" << conn->socketDescriptor() << "because" << kickMessage;
        sendMessage(connection, Messages::STATE_KICKED, kickMessage);
        connection->close(); // got to make sure this causes the connection closed signal ...
    } else {
        sendMessage(connection, Messages::ACK);
    }
}

void CGroupServerCommunicator::sendGroupInformation(CGroupClient *connection)
{
    GroupSelection *selection = getGroup()->selectAll();
    for (const auto character : *selection) {
        // Only send group information for other characters
        if (clientsList.value(character->getName()) == connection->socketDescriptor()) {
            continue;
        }
        // Only share self if we enabled it
        if (getGroup()->getSelf() == character && !Config().groupManager.shareSelf) {
            continue;
        }
        CGroupCommunicator::sendCharUpdate(connection, character->toXML());
    }
    getGroup()->unselect(selection);
}

void CGroupServerCommunicator::sendRemoveUserNotification(CGroupClient *connection,
                                                          const QByteArray &name)
{
    //    qInfo("[Server] Sending remove user notification!");
    GroupSelection *selection = getGroup()->selectByName(name);
    QDomNode blob = selection->value(name)->toXML();
    getGroup()->unselect(selection);
    QByteArray message = formMessageBlock(Messages::REMOVE_CHAR, blob);
    server->sendToAllExceptOne(connection, message);
}

void CGroupServerCommunicator::sendGroupTellMessage(QDomElement root)
{
    QByteArray message = formMessageBlock(Messages::GTELL, root);
    server->sendToAll(message);
}

void CGroupServerCommunicator::relayMessage(CGroupClient *connection,
                                            Messages message,
                                            const QDomNode &data)
{
    QByteArray buffer = formMessageBlock(message, data);

    //  qInfo("Relaying message from %s", (const char *) clientsList.key(connection->socketDescriptor()) );
    server->sendToAllExceptOne(connection, buffer);
}

void CGroupServerCommunicator::sendCharRename(QDomNode blob)
{
    QByteArray message = formMessageBlock(Messages::RENAME_CHAR, blob);
    server->sendToAll(message);
}

void CGroupServerCommunicator::renameConnection(const QByteArray &oldName, const QByteArray &newName)
{
    if (clientsList.contains(oldName)) {
        auto socket = clientsList.value(oldName);
        clientsList.remove(oldName);
        clientsList.insert(newName, socket);
    }

    CGroupCommunicator::renameConnection(oldName, newName);
}

void CGroupServerCommunicator::disconnect()
{
    if (server != nullptr) {
        server->closeAll();
    }
    clientsList.clear();
    emit scheduleAction(new ResetCharacters());
}

void CGroupServerCommunicator::reconnect()
{
    disconnect();
    server = new CGroupServer(this);
    const auto localPort = static_cast<quint16>(Config().groupManager.localPort);
    emit sendLog(QString("Listening on port %1").arg(localPort));
    if (!server->listen(QHostAddress::Any, localPort)) {
        emit sendLog("Failed to start a group Manager server");
        serverStartupFailed();
    }
}

//
// ******************** C L I E N T   S I D E ******************
//
// Client side of the communication protocol
CGroupClientCommunicator::CGroupClientCommunicator(QObject *parent)
    : CGroupCommunicator(GroupManagerState::Client, parent)
{
    emit sendLog("Client mode has been selected");

    client = new CGroupClient(Config().groupManager.host, Config().groupManager.remotePort, this);
}

CGroupClientCommunicator::~CGroupClientCommunicator()
{
    if (client != nullptr) {
        client->disconnectFromHost();
        delete client;
        client = nullptr;
    }
}

void CGroupClientCommunicator::connectionClosed(CGroupClient * /*connection*/)
{
    emit messageBox("Server closed the connection");
    disconnect();
    emit networkDown();
}

void CGroupClientCommunicator::errorInConnection(CGroupClient *connection,
                                                 const QString &errorString)
{
    //    qInfo() << "errorInConnection:" << errorString;
    QString str;

    switch (connection->error()) {
    case QAbstractSocket::ConnectionRefusedError:
        str = QString("Tried to connect to %1 on port %2")
                  .arg(connection->peerName())
                  .arg(Config().groupManager.remotePort);
        emit messageBox(QString("Connection refused: %1.").arg(str));
        break;
    case QAbstractSocket::RemoteHostClosedError:
        emit messageBox(QString("Connection error: %1.").arg(errorString));
        break;
    case QAbstractSocket::HostNotFoundError:
        str = QString("Host %1 not found ").arg(connection->peerName());
        emit messageBox(QString("Connection refused: %1.").arg(str));
        break;
    case QAbstractSocket::SocketAccessError:
    case QAbstractSocket::UnsupportedSocketOperationError:
    case QAbstractSocket::ProxyAuthenticationRequiredError:
    case QAbstractSocket::UnknownSocketError:
    case QAbstractSocket::UnfinishedSocketOperationError:
    default:
        emit messageBox(QString("Connection error: %1.").arg(errorString));
        break;
    }
    disconnect();
    emit networkDown();
}

void CGroupClientCommunicator::retrieveData(CGroupClient *conn, Messages message, QDomNode data)
{
    switch (conn->getConnectionState()) {
    //Closed, Connecting, Connected, Quiting
    case ConnectionStates::Connected:
        // AwaitingLogin, AwaitingInfo, Logged

        if (conn->getProtocolState() == ProtocolStates::AwaitingLogin) {
            // Login state. either REQ_LOGIN or ACK should come
            if (message == Messages::REQ_LOGIN) {
                sendLoginInformation(conn);
            } else if (message == Messages::ACK) {
                // aha! logged on!
                sendMessage(conn, Messages::REQ_INFO);
                conn->setProtocolState(ProtocolStates::AwaitingInfo);
            } else if (message == Messages::STATE_KICKED) {
                // woops
                auto *manager = dynamic_cast<Mmapper2Group *>(parent());
                manager->gotKicked(data);

            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
            }

        } else if (conn->getProtocolState() == ProtocolStates::AwaitingInfo) {
            // almost connected. awaiting full information about the connection
            if (message == Messages::UPDATE_CHAR) {
                emit scheduleAction(new AddCharacter(data.firstChildElement()));
            } else if (message == Messages::STATE_LOGGED) {
                conn->setProtocolState(ProtocolStates::Logged);
            } else if (message == Messages::REQ_ACK) {
                sendMessage(conn, Messages::ACK);
            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
            }

        } else if (conn->getProtocolState() == ProtocolStates::Logged) {
            if (message == Messages::ADD_CHAR) {
                emit scheduleAction(new AddCharacter(data.firstChildElement()));
            } else if (message == Messages::REMOVE_CHAR) {
                emit scheduleAction(new RemoveCharacter(data.firstChildElement()));
            } else if (message == Messages::UPDATE_CHAR) {
                emit scheduleAction(new UpdateCharacter(data.firstChildElement()));
            } else if (message == Messages::RENAME_CHAR) {
                emit scheduleAction(new RenameCharacter(data.firstChildElement()));
            } else if (message == Messages::GTELL) {
                emit gTellArrived(data);
            } else if (message == Messages::REQ_ACK) {
                sendMessage(conn, Messages::ACK);
            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(Logged) Unexpected message marker. Trying to ignore.");
            }
        }

        break;
    case ConnectionStates::Closed:
        qWarning("(Closed) Data arrival during wrong connection state.");
        break;
    case ConnectionStates::Connecting:
        qWarning("(Connecting) Data arrival during wrong connection state.");
        break;
    case ConnectionStates::Quiting:
        qWarning("(Quiting) Data arrival during wrong connection state.");
        break;
    }
}

//
// Parsers and Senders of information and signals to upper and lower objects
//
void CGroupClientCommunicator::sendLoginInformation(CGroupClient *connection)
{
    QDomDocument doc("datagraminfo");

    QDomElement root = doc.createElement("loginData");
    root.setAttribute("protocolVersion", protocolVersion);
    doc.appendChild(root);

    CGroupChar *character = getGroup()->getSelf();
    QDomNode xml = character->toXML();

    root.appendChild(doc.importNode(xml, true));
    sendMessage(connection, Messages::UPDATE_CHAR, root);
}

void CGroupClientCommunicator::sendGroupTellMessage(QDomElement root)
{
    sendMessage(client, Messages::GTELL, root);
}

void CGroupClientCommunicator::sendCharUpdate(QDomNode blob)
{
    CGroupCommunicator::sendCharUpdate(client, blob);
}

void CGroupClientCommunicator::sendCharRename(QDomNode blob)
{
    sendMessage(client, Messages::RENAME_CHAR, blob);
}

void CGroupClientCommunicator::disconnect()
{
    if (client != nullptr) {
        client->disconnectFromHost();
    }
    emit scheduleAction(new ResetCharacters());
}

void CGroupClientCommunicator::reconnect()
{
    disconnect();
    client = new CGroupClient(Config().groupManager.host, Config().groupManager.remotePort, this);
}

void CGroupCommunicator::relayLog(const QString &str)
{
    emit sendLog(str);
}

CGroup *CGroupCommunicator::getGroup()
{
    auto *group = reinterpret_cast<Mmapper2Group *>(this->parent());
    return group->getGroup();
}
