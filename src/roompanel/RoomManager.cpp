// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "RoomManager.h"

#include "../global/Consts.h"
#include "../global/StringView.h"
#include "../global/float_cast.h"
#include "../proxy/GmcpMessage.h"
#include "RoomMobs.h"

#include <memory>
#include <string>
#include <utility>

#include <QtCore>

RoomManager::RoomManager(QObject *const parent)
    : QObject{parent}
    , m_room{this}
{}

RoomManager::~RoomManager() = default;

void RoomManager::slot_reset()
{
    m_room.resetMobs();
}

void RoomManager::updateWidget()
{
    emit sig_updateWidget();
}

void RoomManager::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (msg.isRoomCharsAdd()) {
        parseGmcpAdd(msg);
    } else if (msg.isRoomCharsRemove()) {
        parseGmcpRemove(msg);
    } else if (msg.isRoomCharsSet()) {
        parseGmcpSet(msg);
    } else if (msg.isRoomCharsUpdate()) {
        parseGmcpUpdate(msg);
    }
}

void RoomManager::parseGmcpAdd(const GmcpMessage &msg)
{
    showGmcp(msg);
    if (!msg.getJsonDocument().has_value() || !msg.getJsonDocument()->getObject()) {
        if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                                 << "containing invalid Json: expecting object, got"
                                 << (msg.getJson() ? msg.getJson()->toQString() : QString());
        }
        return;
    }

    auto obj = msg.getJsonDocument()->getObject().value();
    addMob(obj);
}

void RoomManager::parseGmcpRemove(const GmcpMessage &msg)
{
    showGmcp(msg);
    // payload is a single number (usually followed by a space), not an array or object
    // thus parsed msg.getDocument() is not valid: use msg.getJson() instead
    if (!msg.getJson() || msg.getJson()->getStdStringUtf8().empty()) {
        qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                             << "containing invalid empty payload: expecting number";
        return;
    }
    auto optNum = msg.getJsonDocument()->getInt();
    if (!optNum || optNum.value() <= static_cast<JsonInt>(RoomMobData::NOID)) {
        qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                             << "containing invalid payload: expecting unsigned number, got"
                             << msg.getJson()->toQString();
        return;
    }
    const RoomMob::Id id = static_cast<RoomMob::Id>(optNum.value());
    if (m_room.removeMobById(id)) {
        updateWidget();
    }
}

void RoomManager::parseGmcpSet(const GmcpMessage &msg)
{
    showGmcp(msg);

    const auto &doc = msg.getJsonDocument().value();
    if (!doc.getArray()) {
        if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                                 << "containing invalid Json: expecting array, got"
                                 << (msg.getJson() ? msg.getJson()->toQString() : QString());
        }
        return;
    }
    m_room.resetMobs();
    auto array = doc.getArray().value();
    for (const auto &entry : array) {
        if (auto optObj = entry.getObject()) {
            addMob(optObj.value());
        } else {
            if (m_debug) {
                qWarning().noquote()
                    << "RoomManager received GMCP" << msg.getName().toQString()
                    << "containing invalid Json: expecting array of objects, got [/*...*/"
                    << entry.type() << "/*...*/]";
            }
        }
    }
    updateWidget();
}

void RoomManager::parseGmcpUpdate(const GmcpMessage &msg)
{
    showGmcp(msg);
    const auto &doc = msg.getJsonDocument().value();
    if (!doc.getObject()) {
        if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                                 << "containing invalid Json: expecting object, got"
                                 << (msg.getJson() ? msg.getJson()->toQString() : QString());
        }
        return;
    }
    updateMob(doc.getObject().value());
}

void RoomManager::addMob(const JsonObj &obj)
{
    RoomMobUpdate data;
    if (toMob(obj, data)) {
        m_room.addMob(std::move(data));
        updateWidget();
    }
}

void RoomManager::updateMob(const JsonObj &obj)
{
    RoomMobUpdate data;
    if (toMob(obj, data)) {
        if (m_room.updateMob(std::move(data))) {
            updateWidget();
        }
    }
}

inline void RoomManager::showGmcp(const GmcpMessage &msg) const
{
    if (m_debug) {
        qDebug().noquote() << "RoomManager received GMCP:" << msg.toRawBytes();
    }
}

const QHash<QString, MobFieldEnum> RoomManager::mobFields{
    {"name", MobFieldEnum::NAME},
    {"desc", MobFieldEnum::DESC},
    {"fighting", MobFieldEnum::FIGHTING},
    {"flags", MobFieldEnum::FLAGS},
    {"labels", MobFieldEnum::LABELS},
    {"riding", MobFieldEnum::MOUNT},
    {"driving", MobFieldEnum::MOUNT},
    {"position", MobFieldEnum::POSITION},
    {"weapon", MobFieldEnum::WEAPON},
};

bool RoomManager::toMob(const JsonObj &obj, RoomMobUpdate &data) const
{
    auto optId = obj.getInt("id");
    if (!optId || optId.value() <= static_cast<JsonInt>(RoomMobData::NOID)) {
        if (m_debug) {
            qWarning().noquote().nospace()
                << "RoomManager received GMCP containing invalid Json object field ";
            if (!optId) {
                qWarning().noquote().nospace() << "(missing id)";
            } else {
                qWarning().noquote().nospace() << "{id: " << optId.value() << "}";
            }
        }
        return false;
    }
    data.setId(static_cast<RoomMob::Id>(optId.value()));
    for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
        auto match = mobFields.constFind(iter.first());
        if (match != mobFields.constEnd()) {
            toMobField(iter.second(), data, *match);
        } else if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP containing unknown Json object field {"
                                 << iter.first() << "}";
        }
    }
    return true;
}

void RoomManager::toMobField(const JsonValue &value, RoomMobUpdate &data, const MobFieldEnum i)
{
    if (auto optInt = value.getInt()) {
        data.setField(i, QVariant::fromValue(static_cast<RoomMobData::Id>(optInt.value())));
    } else if (auto optString = value.getString()) {
        data.setField(i, QVariant::fromValue(optString.value()));
    } else if (auto optArray = value.getArray()) {
        const auto arr = optArray.value();
        QString str;
        // MUME sends flags and labels as an array of strings
        for (const JsonValue &item : arr) {
            optString = item.getString();
            if (!optString)
                continue;
            if (!str.isEmpty()) {
                str += char_consts::C_COMMA;
            }
            str += optString.value();
        }
        data.setField(i, QVariant::fromValue(str));
    } else {
        // MUME may send "weapon":false and "fighting":null
    }
    data.setFlags(data.getFlags() | i);
}
