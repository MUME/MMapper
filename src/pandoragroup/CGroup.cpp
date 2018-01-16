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

#include "configuration.h"

#include "CGroup.h"
#include "CGroupChar.h"
#include "groupaction.h"

#include <assert.h>
#include <QDebug>

CGroup::CGroup(QObject *parent) :
    QObject(parent),
    characterLock(QMutex::Recursive)
{
    self = new CGroupChar();
    self->setName(Config().m_groupManagerCharName);
    self->setPosition(0);
    self->setColor(Config().m_groupManagerColor);
    charIndex.push_back(self);

    connect(this, SIGNAL(log(const QString &)), parent, SLOT(sendLog(const QString &)));
}

CGroup::~CGroup()
{
    // Delete all characters including self
    for (uint i = 0; i < charIndex.size(); ++i) {
        delete charIndex[i];
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
    GroupSelection *selection = new GroupSelection(this);
    locks.insert(selection);
    selection->receiveCharacters(this, charIndex);
    return selection;
}

GroupSelection *CGroup::selectByName(const QByteArray &name)
{
    QMutexLocker locker(&characterLock);
    GroupSelection *selection = new GroupSelection(this);
    locks.insert(selection);
    CGroupChar *character = getCharByName(name);
    if (character)
        selection->receiveCharacters(this, {character});
    return selection;
}

void CGroup::resetChars()
{
    QMutexLocker locker(&characterLock);

    emit log("You have left the group.");

    for (uint i = 0; i < charIndex.size(); ++i) {
        if (charIndex[i] != self)
            delete charIndex[i];
    }
    charIndex.clear();
    charIndex.push_back(self);

    emit characterChanged();
}

bool CGroup::addChar(QDomNode node)
{
    QMutexLocker locker(&characterLock);
    CGroupChar *newChar = new CGroupChar();
    newChar->updateFromXML(node);
    if (isNamePresent(newChar->getName()) == true || newChar->getName() == "") {
        emit log(QString("'%1' could not join the group because the name already existed.")
                 .arg(newChar->getName().constData()));
        delete newChar;
        return false;
    } else {
        emit log(QString("'%1' joined the group.").arg(newChar->getName().constData()));
        charIndex.push_back(newChar);
        emit characterChanged();
        return true;
    }
}

void CGroup::removeChar(QByteArray name)
{
    QMutexLocker locker(&characterLock);
    if (name == Config().m_groupManagerCharName) {
        emit log("You cannot delete yourself from the group.");
        return;
    }

    for (std::vector<CGroupChar *>::iterator it = charIndex.begin(); it != charIndex.end(); ++it) {
        CGroupChar *character = *it;
        if (character->getName() == name) {
            emit log(QString("Removing '%1' from the group.").arg(
                         character->getName().constData()));
            charIndex.erase(it);
            delete character;
            emit characterChanged();
            return;
        }
    }
}

void CGroup::removeChar(QDomNode node)
{
    QMutexLocker locker(&characterLock);
    QByteArray name = CGroupChar::getNameFromXML(node);
    if (name == "")
        return;

    removeChar(name);
}

bool CGroup::isNamePresent(QByteArray name)
{
    QMutexLocker locker(&characterLock);
    for (uint i = 0; i < charIndex.size(); i++)
        if (charIndex[i]->getName() == name) {
            return true;
        }

    return false;
}

CGroupChar *CGroup::getCharByName(QByteArray name)
{
    QMutexLocker locker(&characterLock);
    for (uint i = 0; i < charIndex.size(); i++)
        if (charIndex[i]->getName() == name)
            return charIndex[i];

    return NULL;
}

void CGroup::updateChar(QDomNode blob)
{
    CGroupChar *ch;

    ch = getCharByName(CGroupChar::getNameFromXML(blob));
    if (ch == NULL)
        return;

    if (ch->updateFromXML(blob) == true)
        emit characterChanged();
}

void CGroup::renameChar(QDomNode blob)
{
    QMutexLocker locker(&characterLock);
    if (blob.nodeName() != "rename") {
        qWarning() << "nodeName was '" << blob.nodeName() << "' and not 'rename'";
        return;
    }

    QString oldname = blob.toElement().attribute("oldname");
    QString newname = blob.toElement().attribute("newname");

    emit log(QString("Renaming '%1' to '%2'").arg(oldname).arg(newname));

    CGroupChar *ch;
    ch = getCharByName(oldname.toLatin1());
    if (ch == NULL) {
        qWarning() << "Unable to find old name" << oldname;
        return;
    }

    ch->setName(newname.toLatin1());
    emit characterChanged();
}
