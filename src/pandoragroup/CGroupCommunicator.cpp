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
#include "groupauthority.h"
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
    connect(this, &CGroupCommunicator::destroyed, parent, &Mmapper2Group::networkDown);
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
    xml.setAutoFormatting(LOG_MESSAGE_INFO);
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
        xml.writeAttribute("mana", playerData["mana"].toString());
        xml.writeAttribute("maxmana", playerData["maxmana"].toString());
        xml.writeAttribute("name", playerData["name"].toString());
        xml.writeAttribute("color", playerData["color"].toString());
        xml.writeAttribute("hp", playerData["hp"].toString());
        xml.writeAttribute("maxmoves", playerData["maxmoves"].toString());
        xml.writeAttribute("room", playerData["room"].toString());
        xml.writeEndElement();
    };

    switch (message) {
    case Messages::REQ_HANDSHAKE:
        xml.writeStartElement("handshake");
        xml.writeStartElement("protocolVersion");
        xml.writeCharacters(data["protocolVersion"].toString());
        xml.writeEndElement();
        xml.writeEndElement();
        break;

    case Messages::UPDATE_CHAR:
        if (data.contains("loginData") && data["loginData"].canConvert(QMetaType::QVariantMap)) {
            // Client needs to submit loginData and nested playerData
            xml.writeStartElement("loginData");
            const QVariantMap &loginData = data["loginData"].toMap();
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

    case Messages::NONE:
    case Messages::ACK:
    case Messages::REQ_ACK:
    case Messages::REQ_INFO:
    case Messages::REQ_LOGIN:
    case Messages::PROT_VERSION:
    case Messages::STATE_LOGGED:
    case Messages::STATE_KICKED:
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

        case Messages::REQ_HANDSHAKE:
            if (xml.name() == QLatin1String("protocolVersion")) {
                data["protocolVersion"] = xml.readElementText().toLatin1();
            }
            break;
        case Messages::UPDATE_CHAR:
            if (xml.name() == QLatin1String("loginData")) {
                const auto &attributes = xml.attributes();
                if (attributes.hasAttribute("protocolVersion"))
                    data["protocolVersion"] = attributes.value("protocolVersion").toUInt();
                xml.readNextStartElement();
            }
            goto common_update_char; // effectively a fall-thru

        common_update_char:
        case Messages::REMOVE_CHAR:
        case Messages::ADD_CHAR:
            if (xml.name() == QLatin1String("playerData")) {
                const auto &attributes = xml.attributes();
                QVariantMap playerData;
                playerData["hp"] = attributes.value("hp").toInt();
                playerData["maxhp"] = attributes.value("maxhp").toInt();
                playerData["moves"] = attributes.value("moves").toInt();
                playerData["maxmoves"] = attributes.value("maxmoves").toInt();
                playerData["mana"] = attributes.value("mana").toInt();
                playerData["maxmana"] = attributes.value("maxmana").toInt();
                playerData["state"] = attributes.value("state").toUInt();
                playerData["name"] = attributes.value("name").toString().toLatin1();
                playerData["color"] = attributes.value("color").toString();
                playerData["room"] = attributes.value("room").toUInt();
                data["playerData"] = playerData;
            }
            break;

        case Messages::RENAME_CHAR:
            if (xml.name() == QLatin1String("rename")) {
                const auto &attributes = xml.attributes();
                data["oldname"] = attributes.value("oldname").toLatin1();
                data["newname"] = attributes.value("newname").toLatin1();
            }
            break;

        case Messages::NONE:
        case Messages::ACK:
        case Messages::REQ_ACK:
        case Messages::REQ_INFO:
        case Messages::REQ_LOGIN:
        case Messages::PROT_VERSION:
        case Messages::STATE_LOGGED:
        case Messages::STATE_KICKED:
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
    connect(getAuthority(),
            &GroupAuthority::secretRevoked,
            this,
            &CGroupServerCommunicator::onRevokeWhitelist);
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
    disconnect(getAuthority(),
               &GroupAuthority::secretRevoked,
               this,
               &CGroupServerCommunicator::onRevokeWhitelist);
    server.closeAll();
    qInfo() << "Destructed CGroupServerCommunicator";
}

void CGroupServerCommunicator::connectionClosed(CGroupClient *const connection)
{
    const QByteArray &name = clientsList.key(connection);
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
}

void CGroupServerCommunicator::connectionEstablished(CGroupClient *const connection)
{
    QVariantMap handshake;
    handshake["protocolVersion"] = NO_OPEN_SSL ? PROTOCOL_VERSION_102 : PROTOCOL_VERSION_103;
    sendMessage(connection, Messages::REQ_HANDSHAKE, handshake);
}

void CGroupServerCommunicator::retrieveData(CGroupClient *const connection,
                                            const Messages message,
                                            const QVariantMap &data)
{
    // ---------------------- AwaitingLogin  --------------------
    if (connection->getProtocolState() == ProtocolState::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, UPDATE_CHAR, or ACK should come
        if (message == Messages::REQ_HANDSHAKE) {
            // Shaking hands with client
            parseHandshake(connection, data);
        } else if (message == Messages::UPDATE_CHAR) {
            // aha! parse the data
            if (connection->getProtocolVersion() >= PROTOCOL_VERSION_103)
                parseLoginInformation(connection, data);
            else
                parseHandshake(connection, data); // Protocol 102 skips the handshake
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
        }
        // ---------------------- AwaitingInfo  --------------------
    } else if (connection->getProtocolState() == ProtocolState::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == Messages::REQ_INFO) {
            sendGroupInformation(connection);
            sendMessage(connection, Messages::REQ_ACK);
        } else if (message == Messages::ACK) {
            connection->setProtocolState(ProtocolState::Logged);
            sendMessage(connection, Messages::STATE_LOGGED);
            if (!NO_OPEN_SSL && connection->getProtocolVersion() == PROTOCOL_VERSION_102) {
                QVariantMap root;
                root["text"] = QString("WARNING: %1 joined the group with an insecure connection "
                                       "and needs to upgrade MMapper!")
                                   .arg(connection->getName().constData());
                root["from"] = "MMapper";
                sendGroupTellMessage(root);
                emit gTellArrived(root);
            }
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
        }

        // ---------------------- LOGGED --------------------
    } else if (connection->getProtocolState() == ProtocolState::Logged) {
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
}

void CGroupServerCommunicator::sendCharUpdate(const QVariantMap &map)
{
    if (getConfig().groupManager.shareSelf) {
        const QByteArray &message = formMessageBlock(Messages::UPDATE_CHAR, map);
        server.sendToAll(message);
    }
}

void CGroupServerCommunicator::parseHandshake(CGroupClient *const connection,
                                              const QVariantMap &data)
{
    if (!data.contains("protocolVersion") || !data["protocolVersion"].canConvert(QMetaType::UInt)) {
        kickConnection(connection, "Payload did not include the 'protocolVersion' attribute");
        return;
    }
    const ProtocolVersion clientProtocolVersion = data["protocolVersion"].toUInt();
    if (clientProtocolVersion < PROTOCOL_VERSION_102) {
        kickConnection(connection,
                       "Host requires a newer version of the group protocol. "
                       "Please upgrade to the latest MMapper.");
        return;
    }
    if (getConfig().groupManager.requireAuth && clientProtocolVersion == PROTOCOL_VERSION_102) {
        kickConnection(connection,
                       "Host requires authorization. "
                       "Please upgrade to the latest MMapper.");
        return;
    }
    auto supportedProtocolVersion = NO_OPEN_SSL ? PROTOCOL_VERSION_102 : PROTOCOL_VERSION_103;
    if (clientProtocolVersion > supportedProtocolVersion) {
        kickConnection(connection, "Host uses an older version of MMapper and needs to upgrade.");
        return;
    }
    if (clientProtocolVersion == PROTOCOL_VERSION_102) {
        connection->setProtocolVersion(clientProtocolVersion);
        parseLoginInformation(connection, data);
    } else {
        assert(!NO_OPEN_SSL);
        sendMessage(connection, Messages::REQ_LOGIN);
        connection->setProtocolVersion(clientProtocolVersion);
        connection->startServerEncrypted();
    }
}

void CGroupServerCommunicator::parseLoginInformation(CGroupClient *connection,
                                                     const QVariantMap &data)
{
    if (!data.contains("playerData") || !data["playerData"].canConvert(QMetaType::QVariantMap)) {
        kickConnection(connection, "Payload did not include 'playerData' element.");
        return;
    }
    const QVariantMap &playerData = data["playerData"].toMap();
    if (!playerData.contains("name") || !playerData["name"].canConvert(QMetaType::QByteArray)) {
        kickConnection(connection, "Payload did not include 'name' attribute.");
        return;
    }
    connection->setName(playerData["name"].toByteArray());
    const QByteArray name = connection->getName();
    const auto secret = connection->getSecret();
    emit sendLog(QString("'%1' is trying to join the group.").arg(name.constData()));
    if (!secret.isEmpty() && connection->getProtocolVersion() >= PROTOCOL_VERSION_103) {
        emit sendLog(QString("'%1's secret: %2").arg(name.constData()).arg(secret.constData()));
        QString secretStr = QString::fromLatin1(connection->getSecret());
        for (auto target : clientsList.values()) {
            if (secretStr.compare(target->getSecret(), Qt::CaseInsensitive) == 0) {
                kickConnection(target, "Someone reconnected to the server using your secret!");
            }
        }

    } else {
        emit sendLog(
            QString("<b>WARNING:</b> '%1' has no secret and their connection is not encrypted.")
                .arg(name.constData()));
    }
    emit sendLog(QString("'%1's IP address: %2")
                     .arg(name.constData())
                     .arg(connection->getPeerAddress().toString()));
    emit sendLog(QString("'%1's protocol version: %2")
                     .arg(name.constData())
                     .arg(connection->getProtocolVersion()));
    if (getConfig().groupManager.requireAuth && !getAuthority()->isAuthorized(secret)) {
        kickConnection(connection, "Host has not added your secret to their contacts!");
        return;
    }
    if (getGroup()->isNamePresent(name)) {
        kickConnection(connection, "The name you picked is already present!");
        return;
    }

    // Client is allowed to log in
    clientsList.insert(name, connection);

    // Strip protocolVersion from original QVariantMap
    QVariantMap charNode;
    charNode["playerData"] = playerData;
    emit scheduleAction(new AddCharacter(charNode));
    relayMessage(connection, Messages::ADD_CHAR, charNode);
    sendMessage(connection, Messages::ACK);
    connection->setProtocolState(ProtocolState::AwaitingInfo);
}

void CGroupServerCommunicator::sendGroupInformation(CGroupClient *const connection)
{
    GroupSelection *selection = getGroup()->selectAll();
    for (const auto &character : *selection) {
        // Only send group information for other characters
        if (clientsList.value(character->getName()) == connection) {
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
        auto connection = clientsList.value(oldName);
        clientsList.remove(oldName);
        clientsList.insert(newName, connection);
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
    deleteLater();
}

void CGroupServerCommunicator::connectCommunicator()
{
    const auto localPort = static_cast<quint16>(getConfig().groupManager.localPort);
    emit sendLog(QString("Listening on port %1").arg(localPort));
    if (!server.listen(QHostAddress::Any, localPort)) {
        emit sendLog("Failed to start a group Manager server");
        serverStartupFailed();
    }
}

bool CGroupServerCommunicator::kickCharacter(const QByteArray &name)
{
    if (getGroup()->getSelf()->getName() == name) {
        emit messageBox("You can't kick yourself!");
        return false;
    }
    if (auto connection = clientsList[name]) {
        kickConnection(connection, "You have been kicked by the host!");
        return true;
    }
    return false;
}

void CGroupServerCommunicator::kickConnection(CGroupClient *connection, const QByteArray &message)
{
    if (connection->getProtocolVersion() == PROTOCOL_VERSION_102
        && connection->getProtocolState() != ProtocolState::AwaitingLogin) {
        // Protocol 102 does not support kicking outside of AwaitingLogin so we fake it with a group tell
        QVariantMap root;
        root["text"] = QString::fromLatin1(message);
        root["from"] = QString::fromLatin1(getConfig().groupManager.charName);
        sendMessage(connection, Messages::GTELL, message);
        server.closeOne(connection);

    } else {
        sendMessage(connection, Messages::STATE_KICKED, message);
        server.closeOne(connection);
    }
    QString identifier = connection->getName().isEmpty() ? connection->getPeerAddress().toString()
                                                         : connection->getName();
    qDebug() << "Kicking" << identifier << "for" << message;
    emit sendLog(QString("'%1' was kicked: %2").arg(identifier).arg(message.constData()));

    for (const auto &name : clientsList.keys()) {
        const auto target = clientsList[name];
        if (connection == target) {
            clientsList.remove(name);
            if (getGroup()->isNamePresent(name)) {
                sendRemoveUserNotification(nullptr, name);
                emit scheduleAction(new RemoveCharacter(name));
            }
        }
    }
}
void CGroupServerCommunicator::onRevokeWhitelist(const QByteArray &secret)
{
    if (getConfig().groupManager.requireAuth) {
        for (const auto &name : clientsList.keys()) {
            const auto connection = clientsList[name];
            if (QString(secret).compare(connection->getSecret(), Qt::CaseInsensitive) == 0) {
                kickConnection(connection, "You have been removed from the host's contacts!");
            }
        }
    }
}

//
// ******************** C L I E N T   S I D E ******************
//
// Client side of the communication protocol
CGroupClientCommunicator::CGroupClientCommunicator(Mmapper2Group *parent)
    : CGroupCommunicator(GroupManagerState::Client, parent)
    , client(parent->getAuthority(), this)
{
    emit sendLog("Client mode has been selected");
    connect(&client, &CGroupClient::incomingData, this, &CGroupClientCommunicator::incomingData);
    connect(&client, &CGroupClient::sendLog, this, &CGroupClientCommunicator::relayLog);
    connect(&client,
            &CGroupClient::errorInConnection,
            this,
            &CGroupClientCommunicator::errorInConnection);
    connect(&client,
            &CGroupClient::connectionEstablished,
            this,
            &CGroupClientCommunicator::connectionEstablished);
    connect(&client,
            &CGroupClient::connectionClosed,
            this,
            &CGroupClientCommunicator::connectionClosed);
    connect(&client,
            &CGroupClient::connectionEncrypted,
            this,
            &CGroupClientCommunicator::connectionEncrypted);
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
    disconnect(&client,
               &CGroupClient::connectionEstablished,
               this,
               &CGroupClientCommunicator::connectionEstablished);
    disconnect(&client,
               &CGroupClient::connectionClosed,
               this,
               &CGroupClientCommunicator::connectionClosed);
    disconnect(&client,
               &CGroupClient::connectionEncrypted,
               this,
               &CGroupClientCommunicator::connectionEncrypted);
    qInfo() << "Destructed CGroupClientCommunicator";
}

void CGroupClientCommunicator::connectionEstablished(CGroupClient * /*connection*/)
{
    clientConnected = true;
}

void CGroupClientCommunicator::connectionClosed(CGroupClient * /*connection*/)
{
    if (!clientConnected)
        return;

    emit sendLog("Server closed the connection");
    disconnectCommunicator();
}

void CGroupClientCommunicator::errorInConnection(CGroupClient *const connection,
                                                 const QString &errorString)
{
    QString str;

    switch (connection->getSocketError()) {
    case QAbstractSocket::ConnectionRefusedError:
        str = QString("Tried to connect to %1 on port %2")
                  .arg(getConfig().groupManager.host.constData())
                  .arg(getConfig().groupManager.remotePort);
        emit messageBox(QString("Connection refused: %1.").arg(str));
        break;

    case QAbstractSocket::RemoteHostClosedError:
        emit messageBox(QString("Connection closed: %1.").arg(errorString));
        break;

    case QAbstractSocket::HostNotFoundError:
        str = QString("Host %1 not found ").arg(getConfig().groupManager.host.constData());
        emit messageBox(QString("Connection refused: %1.").arg(str));
        break;

    case QAbstractSocket::SocketAccessError:
    case QAbstractSocket::UnsupportedSocketOperationError:
    case QAbstractSocket::ProxyAuthenticationRequiredError:
    case QAbstractSocket::UnknownSocketError:
    case QAbstractSocket::UnfinishedSocketOperationError:
    case QAbstractSocket::SocketResourceError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::DatagramTooLargeError:
    case QAbstractSocket::NetworkError:
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::SocketAddressNotAvailableError:
    case QAbstractSocket::SslHandshakeFailedError:
    case QAbstractSocket::ProxyConnectionRefusedError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:
    case QAbstractSocket::ProxyNotFoundError:
    case QAbstractSocket::ProxyProtocolError:
    case QAbstractSocket::OperationError:
    case QAbstractSocket::SslInternalError:
    case QAbstractSocket::SslInvalidUserDataError:
    case QAbstractSocket::TemporaryError:
        emit messageBox(QString("Connection error: %1.").arg(errorString));
        break;
    }
    disconnectCommunicator();
}

void CGroupClientCommunicator::retrieveData(CGroupClient *conn,
                                            const Messages message,
                                            const QVariantMap &data)
{
    if (message == Messages::STATE_KICKED) {
        emit messageBox(QString("You got kicked! Reason: %1").arg(data["text"].toString()));
        disconnectCommunicator();
        return;
    }
    if (conn->getProtocolState() == ProtocolState::AwaitingLogin) {
        // Login state. either REQ_HANDSHAKE, REQ_LOGIN, or ACK should come
        if (message == Messages::REQ_HANDSHAKE) {
            sendHandshake(conn, data);
        } else if (message == Messages::REQ_LOGIN) {
            assert(!NO_OPEN_SSL);
            conn->setProtocolVersion(proposedProtocolVersion);
            conn->startClientEncrypted();
        } else if (message == Messages::ACK) {
            // aha! logged on!
            sendMessage(conn, Messages::REQ_INFO);
            conn->setProtocolState(ProtocolState::AwaitingInfo);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingLogin) Unexpected message marker. Trying to ignore.");
        }

    } else if (conn->getProtocolState() == ProtocolState::AwaitingInfo) {
        // almost connected. awaiting full information about the connection
        if (message == Messages::UPDATE_CHAR) {
            emit scheduleAction(new AddCharacter(data));
        } else if (message == Messages::STATE_LOGGED) {
            conn->setProtocolState(ProtocolState::Logged);
        } else if (message == Messages::REQ_ACK) {
            sendMessage(conn, Messages::ACK);
        } else {
            // ERROR: unexpected message marker!
            // try to ignore?
            qWarning("(AwaitingInfo) Unexpected message marker. Trying to ignore.");
        }

    } else if (conn->getProtocolState() == ProtocolState::Logged) {
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
}

//
// Parsers and Senders of information and signals to upper and lower objects
//
void CGroupClientCommunicator::sendHandshake(CGroupClient *const connection, const QVariantMap &data)
{
    const auto get_server_protocol_version = [](const auto &data) {
        if (!data.contains("protocolVersion")
            || !data["protocolVersion"].canConvert(QMetaType::UInt)) {
            return PROTOCOL_VERSION_102;
        }
        return data["protocolVersion"].toUInt();
    };
    const auto serverProtocolVersion = get_server_protocol_version(data);
    emit sendLog(QString("Host's protocol version: %1").arg(serverProtocolVersion));
    const auto get_proposed_protocol_version = [](const auto serverProtocolVersion) {
        // Ensure we only pick a protocol within the bounds we understand
        if (NO_OPEN_SSL) {
            return PROTOCOL_VERSION_102;
        } else if (serverProtocolVersion >= PROTOCOL_VERSION_103) {
            return PROTOCOL_VERSION_103;
        } else if (serverProtocolVersion <= PROTOCOL_VERSION_102) {
            return PROTOCOL_VERSION_102;
        }
        return serverProtocolVersion;
    };
    proposedProtocolVersion = get_proposed_protocol_version(serverProtocolVersion);

    if (serverProtocolVersion == PROTOCOL_VERSION_102
        || proposedProtocolVersion == PROTOCOL_VERSION_102) {
        if (!NO_OPEN_SSL && getConfig().groupManager.requireAuth) {
            emit messageBox(
                "Host does not support encryption.\n"
                "Consider disabling \"Require authorization\" under the Group Manager settings "
                "or ask the host to upgrade MMapper.");
            disconnectCommunicator();
            return;
        } else {
            // MMapper 2.0.3 through MMapper 2.6 Protocol 102 does not do a version handshake
            // and goes directly to login
            if (!NO_OPEN_SSL)
                emit sendLog("<b>WARNING:</b> "
                             "Host does not support encryption and your connection is insecure.");

            sendLoginInformation(connection);
        }

    } else {
        QVariantMap handshake;
        handshake["protocolVersion"] = proposedProtocolVersion;
        sendMessage(connection, Messages::REQ_HANDSHAKE, handshake);
    }
}

void CGroupClientCommunicator::sendLoginInformation(CGroupClient *const connection)
{
    const CGroupChar *character = getGroup()->getSelf();
    QVariantMap loginData = character->toVariantMap();
    if (proposedProtocolVersion == PROTOCOL_VERSION_102) {
        // Protocol 102 does handshake and login in one step
        loginData["protocolVersion"] = connection->getProtocolVersion();
        connection->setProtocolVersion(PROTOCOL_VERSION_102);
    }
    QVariantMap root;
    root["loginData"] = loginData;
    sendMessage(connection, Messages::UPDATE_CHAR, root);
}

void CGroupClientCommunicator::connectionEncrypted(CGroupClient *const connection)
{
    const auto secret = connection->getSecret();
    emit sendLog(QString("Host's secret: %1").arg(secret.constData()));
    bool authorized = getAuthority()->isAuthorized(secret);
    if (!getConfig().groupManager.requireAuth || authorized) {
        sendLoginInformation(connection);
    } else {
        // REVISIT: Error message?
        emit messageBox(
            QString("Host's secret is not in your contacts:\n%1").arg(secret.constData()));
        disconnectCommunicator();
    }
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
    clientConnected = false;
    client.disconnectFromHost();
    emit scheduleAction(new ResetCharacters());
    deleteLater();
}

void CGroupClientCommunicator::connectCommunicator()
{
    client.connectToHost();
}

bool CGroupClientCommunicator::kickCharacter(const QByteArray &)
{
    emit messageBox("Clients cannot kick players.");
    return false;
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

GroupAuthority *CGroupCommunicator::getAuthority()
{
    auto *group = reinterpret_cast<Mmapper2Group *>(this->parent());
    return group->getAuthority();
}
