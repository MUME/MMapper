// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroup.h"

#include <cassert>
#include <memory>
#include <QByteArray>
#include <QMessageLogContext>
#include <QMutex>
#include <QObject>
#include <QVariantMap>

#include "../configuration/configuration.h"
#include "../global/roomid.h"
#include "CGroupChar.h"
#include "groupaction.h"
#include "groupselection.h"

CGroup::CGroup(QObject *const parent)
    : QObject(parent)
    , self{CGroupChar::alloc()}
{
    const Configuration::GroupManagerSettings &groupManager = getConfig().groupManager;
    self->setName(groupManager.charName);
    self->setRoomId(INVALID_ROOMID);
    self->setColor(groupManager.color);
    charIndex.push_back(self);
}

/**
 * @brief CGroup::scheduleAction
 * @param action to be scheduled once all locks are gone
 */
void CGroup::slot_scheduleAction(std::shared_ptr<GroupAction> action)
{
    QMutexLocker locker(&characterLock);
    action->schedule(this);
    actionSchedule.push(action);
    if (locks.empty()) {
        executeActions();
    }
}

void CGroup::executeActions()
{
    while (!actionSchedule.empty()) {
        std::shared_ptr<GroupAction> action = actionSchedule.front();
        actionSchedule.pop();
        action->exec();
    }
}

void CGroup::virt_releaseCharacters(GroupRecipient *sender)
{
    QMutexLocker lock(&characterLock);
    assert(sender);
    locks.erase(sender);
    if (locks.empty()) {
        executeActions();
    }
}

std::unique_ptr<GroupSelection> CGroup::selectAll()
{
    QMutexLocker locker(&characterLock);
    auto selection = std::make_unique<GroupSelection>(this);
    locks.insert(selection.get());
    selection->receiveCharacters(this, charIndex);
    return selection;
}

std::unique_ptr<GroupSelection> CGroup::selectByName(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    auto selection = std::make_unique<GroupSelection>(this);
    locks.insert(selection.get());
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
    QMutexLocker locker(&characterLock);

    log("You have left the group.");

    for (const auto &character : charIndex) {
        if (character != self) {
            // TODO: mark character as Zombie ?
        }
    }
    charIndex.clear();
    charIndex.push_back(self);

    characterChanged(true);
}

bool CGroup::addChar(const QVariantMap &map)
{
    QMutexLocker locker(&characterLock);
    auto newChar = CGroupChar::alloc();
    newChar->updateFromVariantMap(map);
    if (isNamePresent(newChar->getName()) || newChar->getName() == "") {
        log(QString("'%1' could not join the group because the name already existed.")
                .arg(newChar->getName().constData()));
        return false;
    }
    log(QString("'%1' joined the group.").arg(newChar->getName().constData()));
    charIndex.push_back(newChar);
    characterChanged(true);
    return true;
}

void CGroup::removeChar(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    if (name == getConfig().groupManager.charName) {
        log("You cannot delete yourself from the group.");
        return;
    }

    for (auto it = charIndex.begin(); it != charIndex.end(); ++it) {
        SharedGroupChar character = *it;
        if (character->getName() == name) {
            log(QString("Removing '%1' from the group.").arg(character->getName().constData()));
            charIndex.erase(it);
            characterChanged(true);
            return;
        }
    }
}

bool CGroup::isNamePresent(const QByteArray &name) const
{
    QMutexLocker locker(&characterLock);

    const QString nameStr = name.simplified();
    for (const auto &character : charIndex) {
        if (nameStr.compare(character->getName(), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

SharedGroupChar CGroup::getCharByName(const QByteArray &name) const
{
    QMutexLocker locker(&characterLock);
    for (const auto &character : charIndex) {
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
    const auto oldRoomId = ch.getRoomId();
    const bool change = ch.updateFromVariantMap(map);
    if (!change) {
        return;
    }

    // Update canvas only if the character moved
    const bool updateCanvas = ch.getRoomId() != oldRoomId;
    characterChanged(updateCanvas);
}

void CGroup::renameChar(const QVariantMap &map)
{
    QMutexLocker locker(&characterLock);
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

    log(QString("Renaming '%1' to '%2'").arg(oldname).arg(newname));

    const auto ch = getCharByName(oldname.toLatin1());
    if (ch == nullptr) {
        qWarning() << "Unable to find old name" << oldname;
        return;
    }

    ch->setName(newname.toLatin1());
    characterChanged(false);
}
