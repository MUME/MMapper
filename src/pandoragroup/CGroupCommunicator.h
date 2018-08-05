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

#ifndef CGROUPCOMMUNICATOR_H_
#define CGROUPCOMMUNICATOR_H_

#include <QByteArray>
#include <QDomNode>
#include <QHash>
#include <QObject>
#include <QString>
#include <QtCore>

#include "mmapper2group.h"

class CGroup;
class CGroupClient;
class CGroupServer;
class GroupAction;

class CGroupCommunicator : public QObject
{
    Q_OBJECT
public:
    explicit CGroupCommunicator(GroupManagerState type, QObject *parent);

    static constexpr const int protocolVersion = 102;

    enum class Messages {
        NONE,
        ACK,
        REQ_VERSION,
        REQ_ACK,
        REQ_LOGIN,
        REQ_INFO,
        PROT_VERSION,
        GTELL,
        STATE_LOGGED,
        STATE_KICKED,
        ADD_CHAR,
        REMOVE_CHAR,
        UPDATE_CHAR,
        RENAME_CHAR
    };

    GroupManagerState getType() const { return type; }
    void sendCharUpdate(CGroupClient *, const QDomNode &);
    void sendMessage(CGroupClient *, Messages, const QByteArray &blob = "");
    void sendMessage(CGroupClient *, Messages, const QDomNode &);
    virtual void renameConnection(const QByteArray &, const QByteArray &);

    virtual void disconnect() = 0;
    virtual void reconnect() = 0;
    virtual void sendGroupTellMessage(QDomElement) = 0;
    virtual void sendCharUpdate(QDomNode) = 0;
    virtual void sendCharRename(QDomNode) = 0;

protected:
    QByteArray formMessageBlock(Messages message, const QDomNode &data);
    CGroup *getGroup();

public slots:
    void incomingData(CGroupClient *, const QByteArray &);
    void sendGTell(const QByteArray &);
    void relayLog(const QString &);
    virtual void retrieveData(CGroupClient *, Messages, QDomNode) = 0;
    virtual void connectionClosed(CGroupClient *) = 0;

signals:
    void networkDown();
    void messageBox(QString message);
    void scheduleAction(GroupAction *action);
    void gTellArrived(QDomNode node);
    void sendLog(const QString &);

private:
    GroupManagerState type = GroupManagerState::Off;
};

class CGroupServerCommunicator : public CGroupCommunicator
{
    Q_OBJECT
    friend class CGroupServer;

public:
    explicit CGroupServerCommunicator(QObject *parent);
    ~CGroupServerCommunicator();

    virtual void renameConnection(const QByteArray &oldName, const QByteArray &newName) override;

protected slots:
    void relayMessage(CGroupClient *connection, Messages message, const QDomNode &data);
    void connectionEstablished(CGroupClient *);
    void retrieveData(CGroupClient *connection, Messages message, QDomNode data) override;
    void connectionClosed(CGroupClient *connection) override;

protected:
    void sendRemoveUserNotification(CGroupClient *connection, const QByteArray &name);
    void sendGroupTellMessage(QDomElement root) override;
    void reconnect() override;
    void disconnect() override;
    void sendCharUpdate(QDomNode blob) override;
    void sendCharRename(QDomNode blob) override;

private:
    void parseLoginInformation(CGroupClient *connection, const QDomNode &data);
    void sendGroupInformation(CGroupClient *connection);
    void serverStartupFailed();

    QHash<QByteArray, qintptr> clientsList{};
    CGroupServer *server = nullptr;
};

class CGroupClientCommunicator : public CGroupCommunicator
{
    Q_OBJECT
    friend class CGroupClient;

public:
    explicit CGroupClientCommunicator(QObject *parent);
    ~CGroupClientCommunicator();

public slots:
    void errorInConnection(CGroupClient *connection, const QString &);
    void retrieveData(CGroupClient *conn, Messages message, QDomNode data) override;
    void connectionClosed(CGroupClient *connection) override;

protected:
    void sendGroupTellMessage(QDomElement root) override;
    void reconnect() override;
    void disconnect() override;
    void sendCharUpdate(QDomNode blob) override;
    void sendCharRename(QDomNode blob) override;

private:
    void sendLoginInformation(CGroupClient *connection);

    CGroupClient *client = nullptr;
};

#endif /*CGROUPCOMMUNICATOR_H_*/
