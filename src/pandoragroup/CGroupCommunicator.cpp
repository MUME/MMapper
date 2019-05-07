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

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "CGroup.h"
#include "GroupServer.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "mmapper2group.h"

using Messages = CGroupCommunicator::Messages;

constexpr const bool LOG_MESSAGE_INFO = false;

CGroupCommunicator::CGroupCommunicator(GroupManagerState mode, Mmapper2Group *parent)
    : QObject(parent)
    , mode(mode)
{}

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
    xml.setCodec("ISO 8859-1");
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
        xml.writeAttribute("prespam", playerData["prespam"].toString());
        xml.writeAttribute("affects", playerData["affects"].toString());
        // REVISIT: Use static keys
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

void CGroupCommunicator::sendMessage(GroupSocket *const socket,
                                     const Messages message,
                                     const QByteArray &map)
{
    QVariantMap root;
    root["text"] = QString::fromLatin1(map);
    sendMessage(socket, message, root);
}

void CGroupCommunicator::sendMessage(GroupSocket *const socket,
                                     const Messages message,
                                     const QVariantMap &node)
{
    socket->sendData(formMessageBlock(message, node));
}

// the core of the protocol
void CGroupCommunicator::incomingData(GroupSocket *const socket, const QByteArray &buff)
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
                playerData["prespam"] = attributes.value("prespam").toString().toLatin1();
                playerData["affects"] = attributes.value("affects").toUInt();
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
    retrieveData(socket, message, data);
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

void CGroupCommunicator::sendCharUpdate(GroupSocket *const socket, const QVariantMap &map)
{
    sendMessage(socket, Messages::UPDATE_CHAR, map);
}

void CGroupCommunicator::sendSelfRename(const QByteArray &oldName, const QByteArray &newName)
{
    QVariantMap root;
    root["oldname"] = QString::fromLatin1(oldName);
    root["newname"] = QString::fromLatin1(newName);
    sendCharRename(root);
}

void CGroupCommunicator::relayLog(const QString &str)
{
    emit sendLog(str);
}

CGroup *CGroupCommunicator::getGroup()
{
    auto *group = checked_dynamic_downcast<Mmapper2Group *>(this->parent());
    return group->getGroup();
}

GroupAuthority *CGroupCommunicator::getAuthority()
{
    auto *group = checked_dynamic_downcast<Mmapper2Group *>(this->parent());
    return group->getAuthority();
}
