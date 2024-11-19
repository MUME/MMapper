// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroup.h"

#include "../configuration/configuration.h"
#include "../global/thread_utils.h"
#include "../map/roomid.h"
#include "CGroupChar.h"
#include "groupaction.h"
#include "groupselection.h"

#include <cassert>
#include <memory>

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QVariantMap>

CGroup::CGroup(QObject *const parent)
    : QObject(parent)
    , m_self{CGroupChar::alloc()}
{
    const Configuration::GroupManagerSettings &groupManager = getConfig().groupManager;
    deref(m_self).init(groupManager.charName, groupManager.color);
    m_charIndex.push_back(m_self);
}

/**
 * @brief CGroup::scheduleAction
 * @param action to be scheduled once all locks are gone
 */
void CGroup::slot_scheduleAction(std::shared_ptr<GroupAction> action)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    deref(action).schedule(this);
    m_actionSchedule.emplace(std::move(action));
    if (m_locks.empty()) {
        executeActions();
    }
}

void CGroup::executeActions()
{
    while (!m_actionSchedule.empty()) {
        std::shared_ptr<GroupAction> action = utils::pop_front(m_actionSchedule);
        deref(action).exec();
    }
}

void CGroup::virt_releaseCharacters(GroupRecipient *const sender)
{
    std::ignore = deref(sender);
    ABORT_IF_NOT_ON_MAIN_THREAD();
    m_locks.erase(sender);
    if (m_locks.empty()) {
        executeActions();
    }
}

std::unique_ptr<GroupSelection> CGroup::selectAll()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    auto selection = std::make_unique<GroupSelection>(this);
    m_locks.insert(selection.get());
    selection->receiveCharacters(this, m_charIndex);
    return selection;
}

std::unique_ptr<GroupSelection> CGroup::selectByName(const QString &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    auto selection = std::make_unique<GroupSelection>(this);
    m_locks.insert(selection.get());
    const SharedGroupChar ch = getCharByName(name);
    if (ch == nullptr) {
        return selection;
    }

    GroupVector v;
    v.emplace_back(ch);
    selection->receiveCharacters(this, v);
    return selection;
}

void CGroup::resetChars()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    log("You have left the group.");

    auto &charIndex = m_charIndex;
    for (const auto &character : charIndex) {
        if (character != m_self) {
            // TODO: mark character as Zombie ?
        }
    }
    charIndex.clear();
    charIndex.push_back(m_self);

    characterChanged(true);
}

void CGroup::addChar(const QVariantMap &map)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    auto newChar = CGroupChar::alloc();
    std::ignore = newChar->updateFromVariantMap(map); // why is the return value ignored?
    if (isNamePresent(newChar->getName()) || newChar->getName() == "") {
        log(QString("'%1' could not join the group because the name already existed.")
                .arg(newChar->getName()));
        return; // not added
    }

    log(QString("'%1' joined the group.").arg(newChar->getName()));
    m_charIndex.push_back(newChar);
    characterChanged(true);
}

void CGroup::removeChar(const QString &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (name == getConfig().groupManager.charName) {
        log("You cannot delete yourself from the group.");
        return;
    }

    utils::erase_if(m_charIndex, [this, &name](const SharedGroupChar &pChar) -> bool {
        auto &character = deref(pChar);
        if (character.getName() != name) {
            return false;
        }
        log(QString("Removing '%1' from the group.").arg(character.getName()));
        characterChanged(true);
        return true;
    });
}

bool CGroup::isNamePresent(const QString &name) const
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    const QString nameStr = name.simplified();
    for (const auto &character : m_charIndex) {
        if (nameStr.compare(character->getName(), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

SharedGroupChar CGroup::getCharByName(const QString &name) const
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    for (const auto &character : m_charIndex) {
        if (character->getName() == name) {
            return character;
        }
    }
    return {};
}

void CGroup::updateChar(const QVariantMap &map)
{
    const auto sharedCh = getCharByName(CGroupChar::getNameFromUpdateChar(map));
    if (sharedCh == nullptr) {
        return;
    }

    CGroupChar &ch = *sharedCh;
    const auto oldExternalId = ch.getExternalId();
    const auto oldServerId = ch.getServerId();
    const bool change = ch.updateFromVariantMap(map);
    if (!change) {
        return;
    }

    // Update canvas only if the character moved
    const bool updateCanvas = [oldExternalId, oldServerId, &ch]() -> bool {
        if (ch.getServerId() != INVALID_SERVER_ROOMID) {
            return ch.getServerId() != oldServerId;
        }
        if (ch.getExternalId() != INVALID_EXTERNAL_ROOMID) {
            return ch.getExternalId() != oldExternalId;
        }
        return false;
    }();
    characterChanged(updateCanvas);
}

void CGroup::renameChar(const QVariantMap &map)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (!map.contains("oldname") && map["oldname"].canConvert(QMetaType::QString)) {
        qWarning() << "'oldname' element not found" << map;
        return;
    }
    if (!map.contains("newname") && map["newname"].canConvert(QMetaType::QString)) {
        qWarning() << "'newname' element not found" << map;
        return;
    }

    const QString oldname = map["oldname"].toString();
    const QString newname = map["newname"].toString();

    log(QString("Renaming '%1' to '%2'").arg(oldname, newname));

    const auto ch = getCharByName(oldname);
    if (ch == nullptr) {
        qWarning() << "Unable to find old name" << oldname;
        return;
    }

    ch->setName(newname);
    characterChanged(false);
}
