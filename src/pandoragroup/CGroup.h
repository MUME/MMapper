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

#include "../global/RuleOf5.h"
#include "CGroupChar.h"
#include "groupselection.h"

class GroupSocket;
class CGroupCommunicator;
class GroupAction;

class CGroup final : public QObject, public GroupAdmin
{
    Q_OBJECT

public:
    explicit CGroup(QObject *parent = nullptr);
    ~CGroup() final = default;
    DELETE_CTORS_AND_ASSIGN_OPS(CGroup);

public:
    NODISCARD bool isNamePresent(const QByteArray &name) const;

private:
    // Interactions with group characters should occur through CGroupSelection due to threading
    void virt_releaseCharacters(GroupRecipient *sender) final;

public:
    void unselect(GroupSelection *s)
    {
        releaseCharacters(s);
        delete s;
    }
    std::unique_ptr<GroupSelection> selectAll();
    std::unique_ptr<GroupSelection> selectByName(const QByteArray &);

private:
    void log(const QString &msg) { emit sig_log(msg); }
    void characterChanged(bool updateCanvas) { emit sig_characterChanged(updateCanvas); }

public slots:
    void slot_scheduleAction(std::shared_ptr<GroupAction> action);

signals:
    void sig_log(const QString &);
    void sig_characterChanged(bool updateCanvas);

protected:
    void executeActions();

public:
    NODISCARD const SharedGroupChar &getSelf() { return self; }
    void renameChar(const QVariantMap &map);
    void resetChars();
    void updateChar(const QVariantMap &map); // updates given char from the map
    void removeChar(const QByteArray &name);
    bool addChar(const QVariantMap &node);

public:
    NODISCARD SharedGroupChar getCharByName(const QByteArray &name) const;

private:
    mutable QMutex characterLock{QMutex::Recursive};
    std::set<GroupRecipient *> locks;
    std::queue<std::shared_ptr<GroupAction>> actionSchedule;
    GroupVector charIndex;
    // deleted in destructor as member of charIndex
    SharedGroupChar self;
};
