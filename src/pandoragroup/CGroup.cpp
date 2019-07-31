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

CGroup::CGroup(QObject *parent)
    : QObject(parent)
    , characterLock(QMutex::Recursive)
    , self{new CGroupChar}
{
    const Configuration::GroupManagerSettings &groupManager = getConfig().groupManager;
    self->setName(groupManager.charName);
    self->setRoomId(DEFAULT_ROOMID);
    self->setColor(groupManager.color);
    charIndex.push_back(self);
}

CGroup::~CGroup()
{
    // Delete all characters including self
    for (auto &character : charIndex) {
        delete character;
    }
    charIndex.clear();
}

/**
 * @brief CGroup::scheduleAction
 * @param action to be scheduled once all locks are gone
 */
void CGroup::scheduleAction(GroupAction *action)
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
        GroupAction *action = actionSchedule.front();
        action->exec();
        delete action;
        actionSchedule.pop();
    }
}

void CGroup::releaseCharacters(GroupRecipient *sender)
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
    std::unique_ptr<GroupSelection> selection(new GroupSelection(this));
    locks.insert(selection.get());
    selection->receiveCharacters(this, charIndex);
    return selection;
}

std::unique_ptr<GroupSelection> CGroup::selectByName(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    std::unique_ptr<GroupSelection> selection(new GroupSelection(this));
    locks.insert(selection.get());
    CGroupChar *character = getCharByName(name);
    if (character != nullptr) {
        selection->receiveCharacters(this, {character});
    }
    return selection;
}

void CGroup::resetChars()
{
    QMutexLocker locker(&characterLock);

    emit log("You have left the group.");

    for (auto &character : charIndex) {
        if (character != self) {
            delete character;
        }
    }
    charIndex.clear();
    charIndex.push_back(self);

    emit characterChanged(true);
}

bool CGroup::addChar(const QVariantMap &map)
{
    QMutexLocker locker(&characterLock);
    auto *newChar = new CGroupChar();
    newChar->updateFromVariantMap(map);
    if (isNamePresent(newChar->getName()) || newChar->getName() == "") {
        emit log(QString("'%1' could not join the group because the name already existed.")
                     .arg(newChar->getName().constData()));
        delete newChar;
        return false;
    }
    emit log(QString("'%1' joined the group.").arg(newChar->getName().constData()));
    charIndex.push_back(newChar);
    emit characterChanged(true);
    return true;
}

void CGroup::removeChar(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    if (name == getConfig().groupManager.charName) {
        emit log("You cannot delete yourself from the group.");
        return;
    }

    for (auto it = charIndex.begin(); it != charIndex.end(); ++it) {
        CGroupChar *character = *it;
        if (character->getName() == name) {
            emit log(QString("Removing '%1' from the group.").arg(character->getName().constData()));
            charIndex.erase(it);
            delete character;
            emit characterChanged(true);
            return;
        }
    }
}

bool CGroup::isNamePresent(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);

    const QString nameStr = name.simplified();
    for (auto &character : charIndex) {
        if (nameStr.compare(character->getName(), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

CGroupChar *CGroup::getCharByName(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    for (auto &character : charIndex) {
        if (character->getName() == name) {
            return character;
        }
    }

    return nullptr;
}

void CGroup::updateChar(const QVariantMap &map)
{
    CGroupChar *ch = getCharByName(CGroupChar::getNameFromUpdateChar(map));
    if (ch == nullptr) {
        return;
    }

    const auto oldRoomId = ch->getRoomId();
    const bool change = ch->updateFromVariantMap(map);
    if (change) {
        // Update canvas only if the character moved
        const bool updateCanvas = ch->getRoomId() != oldRoomId;
        emit characterChanged(updateCanvas);
    }
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

    QString oldname = map["oldname"].toString();
    QString newname = map["newname"].toString();

    emit log(QString("Renaming '%1' to '%2'").arg(oldname).arg(newname));

    CGroupChar *const ch = getCharByName(oldname.toLatin1());
    if (ch == nullptr) {
        qWarning() << "Unable to find old name" << oldname;
        return;
    }

    ch->setName(newname.toLatin1());
    emit characterChanged(false);
}
