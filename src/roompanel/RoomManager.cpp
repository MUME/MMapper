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
    if (!msg.getJsonDocument().has_value() || !msg.getJsonDocument()->isObject()) {
        if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                                 << "containing invalid Json: expecting object, got"
                                 << (msg.getJson() ? msg.getJson()->toQString() : QString());
        }
        return;
    }
    addMob(msg.getJsonDocument()->object());
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
    const std::string &str = msg.getJson()->getStdStringUtf8();
    size_t pos = 0;
    unsigned long num = 0;
    bool bad = false;
    try {
        num = std::stoul(str, &pos, 10);
    } catch (...) {
        bad = true; // str does not contain a valid unsigned long
    }
    const RoomMob::Id id = static_cast<RoomMob::Id>(num);
    if (bad || id == RoomMobData::NOID || static_cast<unsigned long>(id) != num
        || (pos < str.size() && str[pos] > char_consts::C_SPACE)) {
        qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                             << "containing invalid payload: expecting unsigned number, got"
                             << msg.getJson()->toQString();
        return;
    }
    if (m_room.removeMobById(id)) {
        updateWidget();
    }
}

void RoomManager::parseGmcpSet(const GmcpMessage &msg)
{
    showGmcp(msg);
    const auto &doc = msg.getJsonDocument().value();
    if (!doc.isArray()) {
        if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                                 << "containing invalid Json: expecting array, got"
                                 << (msg.getJson() ? msg.getJson()->toQString() : QString());
        }
        return;
    }
    m_room.resetMobs();
    for (QJsonValueRef value : doc.array()) {
        if (value.isObject()) {
            addMob(value.toObject());
        } else {
            if (m_debug) {
                qWarning().noquote()
                    << "RoomManager received GMCP" << msg.getName().toQString()
                    << "containing invalid Json: expecting array of objects, got [/*...*/"
                    << value.type() << "/*...*/]";
            }
        }
    }
    updateWidget();
}

void RoomManager::parseGmcpUpdate(const GmcpMessage &msg)
{
    showGmcp(msg);
    const auto &doc = msg.getJsonDocument().value();
    if (!doc.isObject()) {
        if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP" << msg.getName().toQString()
                                 << "containing invalid Json: expecting object, got"
                                 << (msg.getJson() ? msg.getJson()->toQString() : QString());
        }
        return;
    }
    updateMob(doc.object());
}

void RoomManager::addMob(const QJsonObject &obj)
{
    RoomMobUpdate data;
    if (toMob(obj, data)) {
        m_room.addMob(std::move(data));
        updateWidget();
    }
}

void RoomManager::updateMob(const QJsonObject &obj)
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

bool RoomManager::toMob(const QJsonObject &obj, RoomMobUpdate &data) const
{
    auto iter = obj.constFind("id");
    if (iter == obj.constEnd() || !toMobId(*iter, data)) {
        if (m_debug) {
            auto &&warn = qWarning().noquote().nospace();
            warn << "RoomManager received GMCP containing invalid Json object field ";
            if (iter == obj.constEnd()) {
                warn << "(missing id)";
            } else {
                warn << "{id: " << *iter << "}";
            }
        }
        return false;
    }
    for (iter = obj.constBegin(); iter != obj.constEnd(); ++iter) {
        auto match = mobFields.constFind(iter.key());
        if (match != mobFields.constEnd()) {
            toMobField(*iter, data, *match);
        } else if (m_debug) {
            qWarning().noquote() << "RoomManager received GMCP containing unknown Json object field {"
                                 << iter.key() << ": " << *iter << "}";
        }
    }
    return true;
}

bool RoomManager::toMobId(const QJsonValue &value, RoomMobUpdate &data)
{
    // RoomMob::Id cannot represent negative numbers
    const double d = value.toDouble(-1.0);

    // avoid UB when the result is -1, or if it's otherwise out of bounds.
    if (auto tmp = float_cast::convert_float_to_int<RoomMob::Id>(d)) {
        data.setId(tmp.get_value());
    }

    // check for exact conversion
    return utils::isSameFloat(d, static_cast<double>(data.getId()));
}

void RoomManager::toMobField(const QJsonValue &value, RoomMobUpdate &data, const MobFieldEnum i)
{
    switch (value.type()) {
    case QJsonValue::Double:
        data.setField(i, QVariant::fromValue(static_cast<RoomMobData::Id>(value.toDouble())));
        break;
    case QJsonValue::String:
        data.setField(i, QVariant::fromValue(value.toString()));
        break;
    case QJsonValue::Array: {
        QString str;
        // MUME sends flags and labels as array of strings
        for (QJsonValueRef item : value.toArray()) {
            if (item.isString()) {
                if (!str.isEmpty()) {
                    str += char_consts::C_COMMA;
                }
                str += item.toString();
            }
        }
        data.setField(i, QVariant::fromValue(str));
    } break;
    case QJsonValue::Bool:
    case QJsonValue::Null:
    case QJsonValue::Object:
    case QJsonValue::Undefined:
    default:
        // MUME may send "weapon":false and "fighting":null
        break;
    }
    data.setFlags(data.getFlags() | i);
}
