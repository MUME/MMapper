// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mmapper2group.h"

#include "../configuration/configuration.h"
#include "../global/CaseUtils.h"
#include "../global/Charset.h"
#include "../global/JsonArray.h"
#include "../global/JsonObj.h"
#include "../global/thread_utils.h"
#include "../map/sanitizer.h"
#include "../proxy/GmcpMessage.h"
#include "CGroupChar.h"
#include "mmapper2character.h"

#include <memory>

#include <QColor>
#include <QDateTime>
#include <QMessageLogContext>
#include <QThread>
#include <QVariantMap>
#include <QtCore>

Mmapper2Group::Mmapper2Group(QObject *const parent)
    : QObject{parent}
    , m_colorGenerator{getConfig().groupManager.color}
    , m_groupManagerApi{std::make_unique<GroupManagerApi>(*this)}
{}

Mmapper2Group::~Mmapper2Group() {}

void Mmapper2Group::characterChanged(bool updateCanvas)
{
    emit sig_updateWidget();
    if (updateCanvas) {
        emit sig_updateMapCanvas();
    }
}

void Mmapper2Group::onReset()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    resetChars();
}

void Mmapper2Group::parseGmcpCharName(const JsonObj &obj)
{
    if (!m_self) {
        return;
    }
    // "Char.Name" "{\"fullname\":\"Gandalf the Grey\",\"name\":\"Gandalf\"}"
    if (auto optName = obj.getString("name")) {
        m_self->setName(CharacterName{optName.value()});
        characterChanged(false);
    }
}

void Mmapper2Group::parseGmcpCharStatusVars(const JsonObj &obj)
{
    parseGmcpCharName(obj);
}

void Mmapper2Group::parseGmcpCharVitals(const JsonObj &obj)
{
    if (!m_self) {
        return;
    }
    // "Char.Vitals {\"hp\":100,\"maxhp\":100,\"mana\":100,\"maxmana\":100,\"mp\":139,\"maxmp\":139}"
    characterChanged(updateChar(m_self, obj));
}

void Mmapper2Group::parseGmcpGroupAdd(const JsonObj &obj)
{
    const auto id = getGroupId(obj);
    characterChanged(updateChar(addChar(id), obj));
}

void Mmapper2Group::parseGmcpGroupUpdate(const JsonObj &obj)
{
    const auto id = getGroupId(obj);
    auto sharedCh = getCharById(id);
    if (!sharedCh) {
        sharedCh = addChar(id);
    }
    characterChanged(updateChar(sharedCh, obj));
}

void Mmapper2Group::parseGmcpGroupRemove(const JsonInt n)
{
    const auto id = GroupId{static_cast<uint32_t>(n)};
    removeChar(id);
}

void Mmapper2Group::parseGmcpGroupSet(const JsonArray &arr)
{
    // Remove old characters (except self)
    resetChars();
    bool change = false;
    for (const auto &entry : arr) {
        if (auto optObj = entry.getObject()) {
            const auto &obj = optObj.value();
            const auto id = getGroupId(obj);
            if (updateChar(addChar(id), obj)) {
                change = true;
            }
        }
    }
    characterChanged(change);
}

void Mmapper2Group::parseGmcpRoomInfo(const JsonObj &obj)
{
    if (!m_self)
        return;

    if (auto optInt = obj.getInt("id")) {
        const auto srvId = ServerRoomId{static_cast<uint32_t>(optInt.value())};
        if (srvId != m_self->getServerId()) {
            m_self->setServerId(srvId);
        }
    }
    if (auto optString = obj.getString("name")) {
        const auto name = CharacterRoomName{optString.value()};
        if (name != m_self->getRoomName()) {
            m_self->setRoomName(name);
            characterChanged(false);
        }
    }
}

void Mmapper2Group::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (!msg.getJsonDocument().has_value()) {
        return;
    }

    auto debug = [&msg]() {
        qDebug() << msg.getName().toQByteArray() << msg.getJson()->toQByteArray();
    };

    if (msg.isGroupRemove()) {
        debug();
        if (auto optInt = msg.getJsonDocument()->getInt()) {
            parseGmcpGroupRemove(optInt.value());
        }
        return;
    } else if (msg.isGroupSet()) {
        debug();
        if (auto optArray = msg.getJsonDocument()->getArray()) {
            parseGmcpGroupSet(optArray.value());
        }
        return;
    }

    auto optObj = msg.getJsonDocument()->getObject();
    if (!optObj) {
        return;
    }
    auto &obj = optObj.value();

    if (msg.isCharVitals()) {
        debug();
        parseGmcpCharVitals(obj);
    } else if (msg.isCharName() || msg.isCharStatusVars()) {
        debug();
        parseGmcpCharName(obj);
    } else if (msg.isGroupAdd()) {
        debug();
        parseGmcpGroupAdd(obj);
    } else if (msg.isGroupUpdate()) {
        debug();
        parseGmcpGroupUpdate(obj);
    } else if (msg.isRoomInfo()) {
        debug();
        parseGmcpRoomInfo(obj);
    }
}

void Mmapper2Group::slot_setCharRoomIdEstimated(ServerRoomId /* serverId */,
                                                ExternalRoomId /* externalId */)
{
    if (!m_self)
        return;

    // REVISIT: ParseEvent only stores ASCII so we re-parse GMCP for UTF-8

    // REVISIT: Eliminate external ids?
}

void Mmapper2Group::resetChars()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    log("You have left the group.");

    for (const auto &character : m_charIndex) {
        if (!character->isYou() && character->getColor().isValid()) {
            m_colorGenerator.releaseColor(character->getColor());
        }
    }
    m_self.reset();
    m_charIndex.clear();
    characterChanged(true);
}

SharedGroupChar Mmapper2Group::addChar(const GroupId id)
{
    removeChar(id);
    auto sharedCh = CGroupChar::alloc();
    sharedCh->init(id);
    m_charIndex.push_back(sharedCh);
    return sharedCh;
}

void Mmapper2Group::removeChar(const GroupId id)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    utils::erase_if(m_charIndex, [this, &id](const SharedGroupChar &pChar) -> bool {
        auto &character = deref(pChar);
        if (character.getId() != id) {
            return false;
        }
        if (!pChar->isYou() && character.getColor().isValid()) {
            m_colorGenerator.releaseColor(character.getColor());
        }
        qDebug() << "removing" << id.asUint32() << character.getName().toQString();
        characterChanged(true);
        return true;
    });
}

SharedGroupChar Mmapper2Group::getCharById(const GroupId id) const
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    for (const auto &character : m_charIndex) {
        if (character->getId() == id) {
            return character;
        }
    }
    return {};
}

SharedGroupChar Mmapper2Group::getCharByName(const CharacterName &name) const
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    using namespace charset::conversion;
    const auto a = ::toLowerUtf8(utf8ToAscii(name.getStdStringViewUtf8()));
    for (const auto &character : m_charIndex) {
        const auto b = ::toLowerUtf8(utf8ToAscii(character->getName().getStdStringViewUtf8()));
        if (b == a) {
            return character;
        }
    }
    return {};
}

GroupId Mmapper2Group::getGroupId(const JsonObj &obj)
{
    auto optId = obj.getInt("id");
    if (!optId) {
        return INVALID_GROUPID;
    }
    return GroupId{static_cast<uint32_t>(optId.value())};
}

bool Mmapper2Group::updateChar(SharedGroupChar sharedCh, const JsonObj &obj)
{
    CGroupChar &ch = deref(sharedCh);

    const auto id = ch.getId();
    const auto oldServerId = ch.getServerId();
    const bool change = ch.updateFromGmcp(obj);

    if (ch.isYou() && m_self != sharedCh) {
        m_self.reset();
        // REVISIT: Copy fields over into m_self to not drop other Char GMCP data
        m_self = sharedCh;
        sharedCh->setColor(getConfig().groupManager.color);
        qDebug() << "self is" << id.asUint32() << ch.getName().toQString();
    } else if (!sharedCh->getColor().isValid()) {
        sharedCh->setColor(m_colorGenerator.getNextColor());
        qDebug() << "adding" << id.asUint32() << ch.getName().toQString();
    } else {
        qDebug() << "updating" << id.asUint32() << ch.getName().toQString() << "change?" << change;
    }

    // Update canvas only if the character moved
    return change && ch.getServerId() != INVALID_SERVER_ROOMID && ch.getServerId() != oldServerId;
}
