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

#ifndef CGROUP_H_
#define CGROUP_H_

#include "groupselection.h"

#include <QObject>
#include <QMutex>
#include <QDomNode>
#include <queue>
#include <set>
#include <vector>

class CGroupCommunicator;
class CGroupChar;
class CGroupClient;
class GroupAction;

class CGroup : public QObject, public GroupAdmin
{
    Q_OBJECT

    friend class AddCharacter;
    friend class RemoveCharacter;
    friend class RenameCharacter;
    friend class UpdateCharacter;
    friend class KickCharacter;
    friend class ResetCharacters;
    friend class CGroupClientCommunicator;
    friend class CGroupServerCommunicator;
    friend class Mmapper2Group;

public:
    CGroup(QObject *parent);
    virtual ~CGroup();

    bool isNamePresent(const QByteArray &name);

    // Interactions with group characters should occur through CGroupSelection due to threading
    void releaseCharacters(GroupRecipient *sender);
    void unselect(GroupSelection *s)
    {
        releaseCharacters(s);
        delete s;
    }
    GroupSelection *selectAll();
    GroupSelection *selectByName(const QByteArray &);

public slots:
    void scheduleAction(GroupAction *action);

signals:
    void log(const QString &);
    void characterChanged();

protected:
    void executeActions();

    CGroupChar *getSelf()
    {
        return self;
    }
    void renameChar(const QDomNode &blob);
    void resetChars();
    void updateChar(const QDomNode &blob); // updates given char from the blob
    void removeChar(const QByteArray &name);
    void removeChar(const QDomNode &node);
    bool addChar(const QDomNode &node);

private:
    CGroupChar *getCharByName(const QByteArray &name);

    QMutex characterLock;
    std::set<GroupRecipient *> locks;
    std::queue<GroupAction *> actionSchedule;

    std::vector<CGroupChar *> charIndex;
    CGroupChar      *self;
};

#endif /*CGROUP_H_*/
