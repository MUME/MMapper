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

#include "CGroup.h"

#include <cassert>
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
{
    self = new CGroupLocalChar();
    const Configuration::GroupManagerSettings &groupManager = getConfig().groupManager;
    self->setName(groupManager.charName);
    self->setPosition(DEFAULT_ROOMID);
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

GroupSelection *CGroup::selectAll()
{
    QMutexLocker locker(&characterLock);
    auto *selection = new GroupSelection(this);
    locks.insert(selection);
    selection->receiveCharacters(this, charIndex);
    return selection;
}

GroupSelection *CGroup::selectByName(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    auto *selection = new GroupSelection(this);
    locks.insert(selection);
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

    emit characterChanged();
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
    emit characterChanged();
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
            emit characterChanged();
            return;
        }
    }
}

bool CGroup::isNamePresent(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    for (auto &character : charIndex) {
        if (character->getName() == name) {
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
    CGroupChar *ch = getCharByName(CGroupChar::getNameFromVariantMap(map));
    if (ch == nullptr) {
        return;
    }

    if (ch->updateFromVariantMap(map)) {
        emit characterChanged();
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

    CGroupChar *ch;
    ch = getCharByName(oldname.toLatin1());
    if (ch == nullptr) {
        qWarning() << "Unable to find old name" << oldname;
        return;
    }

    ch->setName(newname.toLatin1());
    emit characterChanged();
}
