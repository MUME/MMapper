#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
