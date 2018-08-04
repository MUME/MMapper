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

#ifndef MMAPPER2GROUP_H
#define MMAPPER2GROUP_H

#include <memory>
#include <QArgument>
#include <QMutex>
#include <QObject>

#include "../expandoracommon/component.h"
#include "../global/roomid.h"

class CGroupCommunicator;
class CGroup;
class QDomNode;
enum class GroupManagerState { Off = 0, Client = 1, Server = 2 };

class Mmapper2Group : public Component
{
public:
    Q_OBJECT

    friend class CGroupClientCommunicator;
    friend class CGroupServerCommunicator;

signals:
    void log(const QString &, const QString &);
    void displayGroupTellEvent(const QByteArray &tell); // sends gtell from local user
    void messageBox(QString title, QString message);
    void groupManagerOff();
    void drawCharacters(); // redraw the opengl screen

public:
    explicit Mmapper2Group();
    virtual ~Mmapper2Group();

    GroupManagerState getType();
    int getGroupSize();

    CGroupCommunicator *getCommunicator() { return network.get(); }
    CGroup *getGroup() { return group.get(); }

public slots:
    void setCharPosition(RoomId pos);
    void setType(GroupManagerState newState);
    void updateSelf(); // changing settings

    void sendGTell(const QByteArray &tell); // sends gtell from local user
    void parseScoreInformation(QByteArray score);
    void parsePromptInformation(QByteArray prompt);

    // Communicator
    void gTellArrived(const QDomNode &node);
    void relayMessageBox(const QString &message);
    void sendLog(const QString &);

protected slots:
    void characterChanged();
    void networkDown();

protected:
    void gotKicked(const QDomNode &message);
    void serverStartupFailed(const QString &message);

    void init() override;

private:
    void issueLocalCharUpdate();

    QMutex networkLock{};
    std::unique_ptr<CGroupCommunicator> network;
    std::unique_ptr<CGroup> group;
};

#endif // MMAPPER2GROUP_H
