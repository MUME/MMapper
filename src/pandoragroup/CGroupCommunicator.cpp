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
#include <QVariantMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../configuration/configuration.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupClient.h"
#include "CGroupServer.h"
#include "groupaction.h"
#include "groupselection.h"
#include "mmapper2group.h"

using Messages = CGroupCommunicator::Messages;

constexpr const bool LOG_MESSAGE_INFO = false;

CGroupCommunicator::CGroupCommunicator(GroupManagerState type, Mmapper2Group *parent)
    : QObject(parent)
    , type(type)
{
    connect(this, &CGroupCommunicator::sendLog, parent, &Mmapper2Group::sendLog);
    connect(this, &CGroupCommunicator::messageBox, parent, &Mmapper2Group::relayMessageBox);
    connect(this, &CGroupCommunicator::gTellArrived, parent, &Mmapper2Group::gTellArrived);
    connect(this, &CGroupCommunicator::networkDown, parent, &Mmapper2Group::networkDown);
    connect(this, &CGroupCommunicator::scheduleAction, parent->getGroup(), &CGroup::scheduleAction);
}

//
// Communication protocol switches and logic
//

//
// Low level. Message forming and messaging
//
QByteArray CGroupCommunicator::formMessageBlock(const Messages message, const QVariantMap &data)
{
    QByteArray block;
    QXmlStreamWriter xml(&block);
    xml.writeStartDocument();
    xml.writeStartElement("datagram");
    xml.writeAttribute("message", QString::number(static_cast<int>(message)));
    xml.writeStartElement("data");

    const auto write_player_data = [](auto &xml, const auto &data) {
        if (!data.contains("playerData") || !data["playerData"].canConvert(QMetaType::QVariantMap)) {
            abort();
        }
        const QVariantMap &playerData = data["playerData"].toMap();
        xml.writeStartElement("playerData");
        xml.writeAttribute("maxhp", playerData["maxhp"].toString());
        xml.writeAttribute("moves", playerData["moves"].toString());
        xml.writeAttribute("state", playerData["state"].toString());
        xml.writeAttribute("maxmana", playerData["maxmana"].toString());
        xml.writeAttribute("name", playerData["name"].toString());
        xml.writeAttribute("color", playerData["color"].toString());
        xml.writeAttribute("hp", playerData["hp"].toString());
        xml.writeAttribute("maxmoves", playerData["maxmoves"].toString());
        xml.writeAttribute("room", playerData["room"].toString());
        xml.writeEndElement();
    };

    switch (message) {
    case Messages::UPDATE_CHAR:
        if (data.contains("loginData") && data["loginData"].canConvert(QMetaType::QVariantMap)) {
            // Client needs to submit loginData and nested playerData
            const QVariantMap &loginData = data["loginData"].toMap();
            xml.writeStartElement("loginData");
            xml.writeAttribute("protocolVersion", loginData["protocolVersion"].toString());
            write_player_data(xml, loginData);
            xml.writeEndElement();
        } else {
            // Server just submits playerData
            write_player_data(xml, data);
        }
        break;
    case Messages::GTELL:
        xml.writeStartElement("gtell");
        xml.writeAttribute("from", data["from"].toString());
        xml.writeCharacters(data["text"].toString());
        xml.writeEndElement();
        break;
    case Messages::REMOVE_CHAR:
    case Messages::ADD_CHAR:
        write_player_data(xml, data);
        break;
    case Messages::RENAME_CHAR:
        xml.writeStartElement("rename");
        xml.writeAttribute("oldname", data["oldname"].toString());
        xml.writeAttribute("newname", data["newname"].toString());
        xml.writeEndElement();
        break;
    default:
        xml.writeTextElement("text", data["text"].toString());
        break;
    }

    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndDocument();

    if (LOG_MESSAGE_INFO)
        qInfo() << "Outgoing message:" << block;
    return block;
}

void CGroupCommunicator::sendMessage(CGroupClient *const connection,
                                     const Messages message,
                                     const QByteArray &map)
{
    QVariantMap root;
    root["text"] = QString::fromLatin1(map);
    sendMessage(connection, message, root);
}

void CGroupCommunicator::sendMessage(CGroupClient *const connection,
                                     const Messages message,
                                     const QVariantMap &node)
{
    connection->sendData(formMessageBlock(message, node));
}

// the core of the protocol
void CGroupCommunicator::incomingData(CGroupClient *const conn, const QByteArray &buff)
{
    if (LOG_MESSAGE_INFO)
        qInfo() << "Incoming message:" << buff;

    QXmlStreamReader xml(buff);
    if (xml.readNextStartElement() && xml.error() != QXmlStreamReader::NoError) {
        qWarning() << "Message cannot be read" << buff;
        return;
    }

    if (xml.name() != QLatin1String("datagram")) {
        qWarning() << "Message does not start with element 'datagram'" << buff;
        return;
    }
    if (xml.attributes().isEmpty() || !xml.attributes().hasAttribute("message")) {
        qWarning() << "'datagram' element did not have a 'message' attribute" << buff;
        return;
    }

    // TODO: need stronger type checking
    const Messages message = static_cast<Messages>(xml.attributes().value("message").toInt());

    if (xml.readNextStartElement() && xml.name() != QLatin1String("data")) {
        qWarning() << "'datagram' element did not have a 'data' child element" << buff;
        return;
    }

    // Deserialize XML
    QVariantMap data;
    while (xml.readNextStartElement()) {
        switch (message) {
        case Messages::GTELL:
            if (xml.name() == QLatin1String("gtell")) {
                data["from"] = xml.attributes().value("from").toString().toLatin1();
                data["text"] = xml.readElementText().toLatin1();
            }
            break;
        case Messages::UPDATE_CHAR:
            if (xml.name() == QLatin1String("loginData")
                && xml.attributes().hasAttribute("protocolVersion")) {
                data["protocolVersion"] = xml.attributes().value("protocolVersion").toInt();
                xml.readNextStartElement();
            }
            // fall through
            // Above comment can be replaced with [[fallthrough]]; in C++17
        case Messages::REMOVE_CHAR:
        case Messages::ADD_CHAR:
            if (xml.name() == QLatin1String("playerData")) {
                QVariantMap playerData;
                playerData["maxhp"] = xml.attributes().value("maxhp").toInt();
                playerData["moves"] = xml.attributes().value("moves").toInt();
                playerData["state"] = xml.attributes().value("state").toUInt();
                playerData["maxmana"] = xml.attributes().value("maxmana").toInt();
                playerData["name"] = xml.attributes().value("name").toString().toLatin1();
                playerData["color"] = xml.attributes().value("color").toString();
                playerData["hp"] = xml.attributes().value("hp").toInt();
                playerData["maxmoves"] = xml.attributes().value("maxmoves").toInt();
                playerData["room"] = xml.attributes().value("room").toUInt();
                data["playerData"] = playerData;
            }
            break;
        case Messages::RENAME_CHAR:
            if (xml.name() == QLatin1String("rename")) {
                data["oldname"] = xml.attributes().value("oldname").toLatin1();
                data["newname"] = xml.attributes().value("newname").toLatin1();
            }
            break;
        default:
            if (xml.name() == QLatin1String("text")) {
                data["text"] = xml.readElementText();
            }
            break;
        }
    }

    // converting a given node to the text form.
    retrieveData(conn, message, data);
}

// this function is for sending gtell from a local user
void CGroupCommunicator::sendGroupTell(const QByteArray &tell)
{
    // form the gtell QVariantMap first.
    QVariantMap root;
    root["text"] = QString::fromLatin1(tell);
    root["from"] = QString::fromLatin1(getConfig().groupManager.charName);
    // depending on the type of this communicator either send to
    // server or send to everyone
    sendGroupTellMessage(root);
}

void CGroupCommunicator::sendCharUpdate(CGroupClient *const connection, const QVariantMap &map)
{
    sendMessage(connection, Messages::UPDATE_CHAR, map);
}

void CGroupCommunicator::renameConnection(const QByteArray &oldName, const QByteArray &newName)
{
    //    qInfo() << "Renaming from" << oldName << "to" << newName;
    QVariantMap root;
    root["oldname"] = QString::fromLatin1(oldName);
    root["newname"] = QString::fromLatin1(newName);

    sendCharRename(root);
}

//
// ******************** S E R V E R   S I D E ******************
//
// Server side of the communication protocol
CGroupServerCommunicator::CGroupServerCommunicator(Mmapper2Group *parent)
    : CGroupCommunicator(GroupManagerState::Server, parent)
    , server(this)
{
    connect(&server, &CGroupServer::sendLog, this, &CGroupServerCommunicator::relayLog);
    connect(&server,
            &CGroupServer::connectionClosed,
            this,
            &CGroupServerCommunicator::connectionClosed);
    emit sendLog("Server mode has been selected");
    connectCommunicator();
}

CGroupServerCommunicator::~CGroupServerCommunicator()
{
    disconnect(&server, &CGroupServer::sendLog, this, &CGroupServerCommunicator::relayLog);
    disconnect(&server,
               &CGroupServer::connectionClosed,
               this,
               &CGroupServerCommunicator::connectionClosed);
    server.closeAll();
    qInfo() << "Destructed CGroupServerCommunicator";
}

void CGroupServerCommunicator::connectionClosed(CGroupClient *const connection)
{
    const auto sock = connection->socketDescriptor();

    const QByteArray &name = clientsList.key(sock);
    if (!name.isEmpty()) {
        sendRemoveUserNotification(connection, name);
        clientsList.remove(name);
        emit sendLog(QString("'%1' closed their connection and quit.").arg(QString(name)));
        emit scheduleAction(new RemoveCharacter(name));
    }
}

void CGroupServerCommunicator::serverStartupFailed()
{
    emit messageBox(
        QString("Failed to start the groupManager server: %1.").arg(server.errorString()));
    disconnectCommunicator();
    emit networkDown();
}

void CGroupServerCommunicator::connectionEstablished(CGroupClient *const connection)
{
    //    qInfo() << "Detected new connection, starting handshake";
    // REVISIT: Provide the server protocol upon login?
    sendMessage(connection, Messages::REQ_LOGIN, "test");
}

void CGroupServerCommunicator::retrieveData(CGroupClient *const connection,
                                            const Messages message,
                                            const QVariantMap &data)
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
                emit scheduleAction(new UpdateCharacter(data));
                relayMessage(connection, Messages::UPDATE_CHAR, data);
            } else if (message == Messages::GTELL) {
                emit gTellArrived(data);
                relayMessage(connection, Messages::GTELL, data);
            } else if (message == Messages::REQ_ACK) {
                sendMessage(connection, Messages::ACK);
            } else if (message == Messages::RENAME_CHAR) {
                emit scheduleAction(new RenameCharacter(data));
                relayMessage(connection, Messages::RENAME_CHAR, data);
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

void CGroupServerCommunicator::sendCharUpdate(const QVariantMap &map)
{
    if (getConfig().groupManager.shareSelf) {
        const QByteArray &message = formMessageBlock(Messages::UPDATE_CHAR, map);
        server.sendToAll(message);
    }
}

void CGroupServerCommunicator::parseLoginInformation(CGroupClient *const connection,
                                                     const QVariantMap &data)
{
    //    qInfo() << "Parsing login information from" << conn->socketDescriptor();
    const auto kick_connection = [this](auto connection, const auto &kickMessage) {
        this->sendMessage(connection, Messages::STATE_KICKED, kickMessage);
        connection->close();
    };
    if (!data.contains("protocolVersion") || !data["protocolVersion"].canConvert(QMetaType::Int)) {
        kick_connection(connection, "Payload did not include the 'protocolVersion' attribute");
        return;
    }
    const int clientProtocolVersion = data["protocolVersion"].toInt();

    if (clientProtocolVersion < SUPPORTED_PROTOCOL_VERSION) {
        // REVISIT: Should we support older versions?
        kick_connection(connection, "Server uses newer version of the protocol. Please update.");
        return;
    }
    if (clientProtocolVersion > SUPPORTED_PROTOCOL_VERSION) {
        kick_connection(connection, "Server uses older version of the protocol.");
        return;
    }
    if (!data.contains("playerData") || !data["playerData"].canConvert(QMetaType::QVariantMap)) {
        kick_connection(connection, "Payload did not include 'playerData' element.");
        return;
    }
    const QVariantMap &playerData = data["playerData"].toMap();
    if (!playerData.contains("name") || !playerData["name"].canConvert(QMetaType::QByteArray)) {
        kick_connection(connection, "Payload did not include 'name' attribute.");
        return;
    }
    const QByteArray &name = playerData["name"].toByteArray();
    if (getGroup()->isNamePresent(name)) {
        kick_connection(connection, "The name you picked is already present!");
        return;
    }
    emit sendLog(
        QString("'%1's protocol version: %2").arg(name.constData()).arg(clientProtocolVersion));

    // Client is allowed to log in
    clientsList.insert(name, connection->socketDescriptor());

    // Strip protocolVersion from original QVariantMap
    QVariantMap charNode;
    charNode["playerData"] = playerData;
    emit scheduleAction(new AddCharacter(charNode));
    relayMessage(connection, Messages::ADD_CHAR, charNode);
    sendMessage(connection, Messages::ACK);
}

void CGroupServerCommunicator::sendGroupInformation(CGroupClient *const connection)
{
    GroupSelection *selection = getGroup()->selectAll();
    for (const auto &character : *selection) {
        // Only send group information for other characters
        if (clientsList.value(character->getName()) == connection->socketDescriptor()) {
            continue;
        }
        // Only share self if we enabled it
        if (getGroup()->getSelf() == character && !getConfig().groupManager.shareSelf) {
            continue;
        }
        CGroupCommunicator::sendCharUpdate(connection, character->toVariantMap());
    }
    getGroup()->unselect(selection);
}

void CGroupServerCommunicator::sendRemoveUserNotification(CGroupClient *const connection,
                                                          const QByteArray &name)
{
    //    qInfo("[Server] Sending remove user notification!");
    GroupSelection *selection = getGroup()->selectByName(name);
    for (const auto &character : *selection) {
        if (character->getName() == name) {
            const QVariantMap &map = character->toVariantMap();
            const QByteArray &message = formMessageBlock(Messages::REMOVE_CHAR, map);
            server.sendToAllExceptOne(connection, message);
        }
    }
    getGroup()->unselect(selection);
}

void CGroupServerCommunicator::sendGroupTellMessage(const QVariantMap &root)
{
    const QByteArray &message = formMessageBlock(Messages::GTELL, root);
    server.sendToAll(message);
}

void CGroupServerCommunicator::relayMessage(CGroupClient *const connection,
                                            const Messages message,
                                            const QVariantMap &data)
{
    const QByteArray &buffer = formMessageBlock(message, data);

    //  qInfo("Relaying message from %s", (const char *) clientsList.key(connection->socketDescriptor()) );
    server.sendToAllExceptOne(connection, buffer);
}

void CGroupServerCommunicator::sendCharRename(const QVariantMap &map)
{
    QByteArray message = formMessageBlock(Messages::RENAME_CHAR, map);
    server.sendToAll(message);
}

void CGroupServerCommunicator::renameConnection(const QByteArray &oldName, const QByteArray &newName)
{
    if (clientsList.contains(oldName)) {
        auto socket = clientsList.value(oldName);
        clientsList.remove(oldName);
        clientsList.insert(newName, socket);
    } else {
        // REVISIT: Kick misbehaving client? (right now it could be ourself!)
    }

    CGroupCommunicator::renameConnection(oldName, newName);
}

void CGroupServerCommunicator::disconnectCommunicator()
{
    server.closeAll();
    clientsList.clear();
    emit scheduleAction(new ResetCharacters());
}

void CGroupServerCommunicator::connectCommunicator()
{
    disconnectCommunicator();
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    emit sendLog(QString("Listening on port %1").arg(localPort));
    if (!server.listen(QHostAddress::Any, localPort)) {
        emit sendLog("Failed to start a group Manager server");
        serverStartupFailed();
    }
}

//
// ******************** C L I E N T   S I D E ******************
//
// Client side of the communication protocol
CGroupClientCommunicator::CGroupClientCommunicator(Mmapper2Group *parent)
    : CGroupCommunicator(GroupManagerState::Client, parent)
    , client(this)
{
    emit sendLog("Client mode has been selected");
    connect(&client, &CGroupClient::incomingData, this, &CGroupClientCommunicator::incomingData);
    connect(&client, &CGroupClient::sendLog, this, &CGroupClientCommunicator::relayLog);
    connect(&client,
            &CGroupClient::errorInConnection,
            this,
            &CGroupClientCommunicator::errorInConnection);
    connectCommunicator();
}

CGroupClientCommunicator::~CGroupClientCommunicator()
{
    client.disconnectFromHost();
    disconnect(&client, &CGroupClient::incomingData, this, &CGroupClientCommunicator::incomingData);
    disconnect(&client, &CGroupClient::sendLog, this, &CGroupClientCommunicator::relayLog);
    disconnect(&client,
               &CGroupClient::errorInConnection,
               this,
               &CGroupClientCommunicator::errorInConnection);
    qInfo() << "Destructed CGroupClientCommunicator";
}

void CGroupClientCommunicator::connectionClosed(CGroupClient * /*connection*/)
{
    emit sendLog("Server closed the connection");
    disconnectCommunicator();
    emit networkDown();
}

void CGroupClientCommunicator::errorInConnection(CGroupClient *const connection,
                                                 const QString &errorString)
{
    //    qInfo() << "errorInConnection:" << errorString;
    QString str;

    switch (connection->error()) {
    case QAbstractSocket::ConnectionRefusedError:
        str = QString("Tried to connect to %1 on port %2")
                  .arg(connection->peerName())
                  .arg(getConfig().groupManager.remotePort);
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
    emit networkDown();
    disconnectCommunicator();
}

void CGroupClientCommunicator::retrieveData(CGroupClient *conn,
                                            const Messages message,
                                            const QVariantMap &data)
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
                conn->close();
                emit networkDown();
                disconnectCommunicator();

            } else {
                // ERROR: unexpected message marker!
                // try to ignore?
                qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
            }

        } else if (conn->getProtocolState() == ProtocolStates::AwaitingInfo) {
            // almost connected. awaiting full information about the connection
            if (message == Messages::UPDATE_CHAR) {
                emit scheduleAction(new AddCharacter(data));
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
                emit scheduleAction(new AddCharacter(data));
            } else if (message == Messages::REMOVE_CHAR) {
                emit scheduleAction(new RemoveCharacter(data));
            } else if (message == Messages::UPDATE_CHAR) {
                emit scheduleAction(new UpdateCharacter(data));
            } else if (message == Messages::RENAME_CHAR) {
                emit scheduleAction(new RenameCharacter(data));
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
void CGroupClientCommunicator::sendLoginInformation(CGroupClient *const connection)
{
    const CGroupChar *character = getGroup()->getSelf();
    QVariantMap loginData = character->toVariantMap();
    int protocolVersion = SUPPORTED_PROTOCOL_VERSION;
    loginData["protocolVersion"] = protocolVersion;

    QVariantMap root;
    root["loginData"] = loginData;

    sendMessage(connection, Messages::UPDATE_CHAR, root);
}

void CGroupClientCommunicator::sendGroupTellMessage(const QVariantMap &root)
{
    sendMessage(&client, Messages::GTELL, root);
}

void CGroupClientCommunicator::sendCharUpdate(const QVariantMap &map)
{
    CGroupCommunicator::sendCharUpdate(&client, map);
}

void CGroupClientCommunicator::sendCharRename(const QVariantMap &map)
{
    sendMessage(&client, Messages::RENAME_CHAR, map);
}

void CGroupClientCommunicator::disconnectCommunicator()
{
    client.abort();
    client.disconnectFromHost();
    emit scheduleAction(new ResetCharacters());
}

void CGroupClientCommunicator::connectCommunicator()
{
    if (client.getConnectionState() != ConnectionStates::Closed) {
        disconnectCommunicator();
    }

    client.setConnectionState(ConnectionStates::Connecting);
    client.setProtocolState(ProtocolStates::AwaitingLogin);
    client.connectToHost(getConfig().groupManager.host,
                         static_cast<quint16>(getConfig().groupManager.remotePort));
    if (!client.waitForConnected(5000)) {
        if (client.getConnectionState() == ConnectionStates::Connecting) {
            client.setConnectionState(ConnectionStates::Quiting);
            errorInConnection(&client, client.errorString());
        }
    } else {
        // Linux needs to have this option set after the server has established a connection
        client.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    }
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
