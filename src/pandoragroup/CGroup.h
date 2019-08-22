#pragma once
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

#include <memory>
#include <set>
#include <vector>
#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QVariantMap>
#include <QWidget>
#include <QtCore>
#include <queue>

#include "groupselection.h"

class CGroupChar;
class GroupSocket;
class CGroupCommunicator;
class GroupAction;

class CGroup final : public QObject, public GroupAdmin
{
    Q_OBJECT

    friend class AddCharacter;
    friend class RemoveCharacter;
    friend class RenameCharacter;
    friend class UpdateCharacter;
    friend class ResetCharacters;
    friend class GroupClient;
    friend class GroupServer;
    friend class Mmapper2Group;

public:
    explicit CGroup(QObject *parent);
    virtual ~CGroup() override;

    bool isNamePresent(const QByteArray &name);

    // Interactions with group characters should occur through CGroupSelection due to threading
    void releaseCharacters(GroupRecipient *sender) override;
    void unselect(GroupSelection *s)
    {
        releaseCharacters(s);
        delete s;
    }
    std::unique_ptr<GroupSelection> selectAll();
    std::unique_ptr<GroupSelection> selectByName(const QByteArray &);

public slots:
    void scheduleAction(GroupAction *action);

signals:
    void log(const QString &);
    void characterChanged(bool updateCanvas);

protected:
    void executeActions();

    CGroupChar *getSelf() { return self; }
    void renameChar(const QVariantMap &map);
    void resetChars();
    void updateChar(const QVariantMap &map); // updates given char from the map
    void removeChar(const QByteArray &name);
    bool addChar(const QVariantMap &node);

private:
    CGroupChar *getCharByName(const QByteArray &name);

    QMutex characterLock;
    std::set<GroupRecipient *> locks;
    std::queue<GroupAction *> actionSchedule;

    std::vector<CGroupChar *> charIndex;
    CGroupChar *self;
};
