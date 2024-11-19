// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupCommunicator.h"

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "CGroup.h"
#include "GroupServer.h"
#include "GroupSocket.h"
#include "groupaction.h"
#include "groupauthority.h"
#include "mmapper2group.h"

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using MessagesEnum = CGroupCommunicator::MessagesEnum;

constexpr const bool LOG_MESSAGE_INFO = false;

CGroupCommunicator::CGroupCommunicator(const GroupManagerStateEnum mode, Mmapper2Group *const parent)
    : QObject{parent}
    , m_mode{mode}
{}

//
// Communication protocol switches and logic
//

//
// Low level. Message forming and messaging
//
QByteArray CGroupCommunicator::formMessageBlock(const MessagesEnum message, const QVariantMap &data)
{
    QByteArray block;
    QXmlStreamWriter xml(&block);
    xml.setCodec("ISO 8859-1");
    xml.setAutoFormatting(LOG_MESSAGE_INFO);
    xml.writeStartDocument();
    xml.writeStartElement("datagram");
    xml.writeAttribute("message", QString::number(static_cast<int>(message)));
    xml.writeStartElement("data");

    const auto write_player_data = [&xml](const QVariantMap &output_data) {
        if (!output_data.contains("playerData")
            || !output_data["playerData"].canConvert(QMetaType::QVariantMap)) {
            throw std::runtime_error("playerData not a map");
        }
        const QVariantMap &playerData = output_data["playerData"].toMap();
        xml.writeStartElement("playerData");
        xml.writeAttribute("maxhp", playerData["maxhp"].toString());
        xml.writeAttribute("moves", playerData["moves"].toString());
        xml.writeAttribute("state", playerData["state"].toString());
        xml.writeAttribute("mana", playerData["mana"].toString());
        xml.writeAttribute("maxmana", playerData["maxmana"].toString());
        xml.writeAttribute("name", playerData["name"].toString());
        xml.writeAttribute("label", playerData["label"].toString());
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
    case MessagesEnum::REQ_HANDSHAKE:
        xml.writeStartElement("handshake");
        xml.writeStartElement("protocolVersion");
        xml.writeCharacters(data["protocolVersion"].toString());
        xml.writeEndElement();
        xml.writeEndElement();
        break;

    case MessagesEnum::UPDATE_CHAR:
        if (data.contains("loginData") && data["loginData"].canConvert(QMetaType::QVariantMap)) {
            // Client needs to submit loginData and nested playerData
            xml.writeStartElement("loginData");
            const QVariantMap &loginData = data["loginData"].toMap();
            xml.writeAttribute("protocolVersion", loginData["protocolVersion"].toString());
            write_player_data(loginData);
            xml.writeEndElement();
        } else {
            // Server just submits playerData
            write_player_data(data);
        }
        break;

    case MessagesEnum::GTELL:
        xml.writeStartElement("gtell");
        xml.writeAttribute("from", data["from"].toString());
        xml.writeCharacters(data["text"].toString());
        xml.writeEndElement();
        break;

    case MessagesEnum::REMOVE_CHAR:
    case MessagesEnum::ADD_CHAR:
        write_player_data(data);
        break;

    case MessagesEnum::RENAME_CHAR:
        xml.writeStartElement("rename");
        xml.writeAttribute("oldname", data["oldname"].toString());
        xml.writeAttribute("newname", data["newname"].toString());
        xml.writeEndElement();
        break;

    case MessagesEnum::NONE:
    case MessagesEnum::ACK:
    case MessagesEnum::REQ_ACK:
    case MessagesEnum::REQ_INFO:
    case MessagesEnum::REQ_LOGIN:
    case MessagesEnum::PROT_VERSION:
    case MessagesEnum::STATE_LOGGED:
    case MessagesEnum::STATE_KICKED:
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

void CGroupCommunicator::sendMessage(GroupSocket &socket,
                                     const MessagesEnum message,
                                     const QByteArray &text)
{
    QVariantMap root;
    root["text"] = QString::fromLatin1(text);
    sendMessage(socket, message, root);
}

void CGroupCommunicator::sendMessage(GroupSocket &socket,
                                     const MessagesEnum message,
                                     const QVariantMap &node)
{
    socket.sendData(formMessageBlock(message, node));
}

// the core of the protocol
void CGroupCommunicator::slot_incomingData(GroupSocket *const socket, const QByteArray &buff)
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
    const MessagesEnum message = static_cast<MessagesEnum>(
        xml.attributes().value("message").toInt());

    if (xml.readNextStartElement() && xml.name() != QLatin1String("data")) {
        qWarning() << "'datagram' element did not have a 'data' child element" << buff;
        return;
    }

    // Deserialize XML
    QVariantMap data;
    while (xml.readNextStartElement()) {
        switch (message) {
        case MessagesEnum::GTELL:
            if (xml.name() == QLatin1String("gtell")) {
                data["from"] = xml.attributes().value("from").toString();
                data["text"] = xml.readElementText();
            }
            break;

        case MessagesEnum::REQ_HANDSHAKE:
            if (xml.name() == QLatin1String("protocolVersion")) {
                data["protocolVersion"] = xml.readElementText();
            }
            break;
        case MessagesEnum::UPDATE_CHAR:
            if (xml.name() == QLatin1String("loginData")) {
                const auto &attributes = xml.attributes();
                if (attributes.hasAttribute("protocolVersion"))
                    data["protocolVersion"] = attributes.value("protocolVersion").toUInt();
                xml.readNextStartElement();
            }
            goto common_update_char; // effectively a fall-thru

        common_update_char:
        case MessagesEnum::REMOVE_CHAR:
        case MessagesEnum::ADD_CHAR:
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
                playerData["name"] = attributes.value("name").toString();
                playerData["label"] = attributes.value("label").toString();
                playerData["color"] = attributes.value("color").toString();
                playerData["room"] = attributes.value("room").toUInt();
                playerData["prespam"] = attributes.value("prespam").toString();
                playerData["affects"] = attributes.value("affects").toUInt();
                data["playerData"] = playerData;
            }
            break;

        case MessagesEnum::RENAME_CHAR:
            if (xml.name() == QLatin1String("rename")) {
                const auto &attributes = xml.attributes();
                data["oldname"] = attributes.value("oldname").toString();
                data["newname"] = attributes.value("newname").toString();
            }
            break;

        case MessagesEnum::NONE:
        case MessagesEnum::ACK:
        case MessagesEnum::REQ_ACK:
        case MessagesEnum::REQ_INFO:
        case MessagesEnum::REQ_LOGIN:
        case MessagesEnum::PROT_VERSION:
        case MessagesEnum::STATE_LOGGED:
        case MessagesEnum::STATE_KICKED:
            if (xml.name() == QLatin1String("text")) {
                data["text"] = xml.readElementText();
            }
            break;
        }
    }

    // converting a given node to the text form.
    slot_retrieveData(socket, message, data);
}

// this function is for sending gtell from a local user
void CGroupCommunicator::slot_sendGroupTell(const QByteArray &tell)
{
    // form the gtell QVariantMap first.
    QVariantMap root;
    root["text"] = QString::fromLatin1(tell);
    root["from"] = QString::fromLatin1(getGroup()->getSelf()->getName());
    // depending on the type of this communicator either send to
    // server or send to everyone
    slot_sendGroupTellMessage(root);
}

void CGroupCommunicator::sendCharUpdate(GroupSocket &socket, const QVariantMap &map)
{
    sendMessage(socket, MessagesEnum::UPDATE_CHAR, map);
}

void CGroupCommunicator::slot_sendSelfRename(const QByteArray &oldName, const QByteArray &newName)
{
    QVariantMap root;
    root["oldname"] = QString::fromLatin1(oldName);
    root["newname"] = QString::fromLatin1(newName);
    slot_sendCharRename(root);
}

void CGroupCommunicator::slot_relayLog(const QString &str)
{
    emit sig_sendLog(str);
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
