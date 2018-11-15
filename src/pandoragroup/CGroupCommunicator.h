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
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtCore>

#include "CGroupClient.h"
#include "CGroupServer.h"
#include "mmapper2group.h"

class CGroup;
class GroupAction;

class CGroupCommunicator : public QObject
{
    Q_OBJECT
public:
    explicit CGroupCommunicator(GroupManagerState type, Mmapper2Group *parent);

    static constexpr const int SUPPORTED_PROTOCOL_VERSION = 102;

    // TODO: password and encryption options
    enum class Messages {
        NONE, // Unused
        ACK,
        REQ_VERSION, // Unused
        REQ_ACK,
        REQ_LOGIN,
        REQ_INFO,
        PROT_VERSION, // Unused
        GTELL,
        STATE_LOGGED,
        STATE_KICKED,
        ADD_CHAR,
        REMOVE_CHAR,
        UPDATE_CHAR,
        RENAME_CHAR
    };

    GroupManagerState getType() const { return type; }
    void sendCharUpdate(CGroupClient *, const QVariantMap &);
    virtual void renameConnection(const QByteArray &, const QByteArray &);

    virtual void disconnectCommunicator() = 0;
    virtual void connectCommunicator() = 0;
    virtual void sendCharUpdate(const QVariantMap &map) = 0;
    virtual bool kickCharacter(const QByteArray &) = 0;

protected:
    void sendMessage(CGroupClient *, const Messages, const QByteArray & = "");
    void sendMessage(CGroupClient *, const Messages, const QVariantMap &);

    virtual void sendGroupTellMessage(const QVariantMap &map) = 0;
    virtual void sendCharRename(const QVariantMap &map) = 0;

    QByteArray formMessageBlock(const Messages message, const QVariantMap &data);
    CGroup *getGroup();

public slots:
    void incomingData(CGroupClient *, const QByteArray &);
    void sendGroupTell(const QByteArray &);
    void relayLog(const QString &);
    virtual void retrieveData(CGroupClient *, const Messages, const QVariantMap &) = 0;
    virtual void connectionClosed(CGroupClient *) = 0;

signals:
    void networkDown();
    void messageBox(QString message);
    void scheduleAction(GroupAction *action);
    void gTellArrived(QVariantMap node);
    void sendLog(const QString &);

private:
    GroupManagerState type = GroupManagerState::Off;
};

class CGroupServerCommunicator final : public CGroupCommunicator
{
    Q_OBJECT
    friend class CGroupServer;

public:
    explicit CGroupServerCommunicator(Mmapper2Group *parent);
    ~CGroupServerCommunicator() override;

    virtual void renameConnection(const QByteArray &oldName, const QByteArray &newName) override;

protected slots:
    void relayMessage(CGroupClient *connection, const Messages message, const QVariantMap &data);
    void connectionEstablished(CGroupClient *connection);
    void retrieveData(CGroupClient *connection,
                      const Messages message,
                      const QVariantMap &data) override;
    void connectionClosed(CGroupClient *connection) override;

protected:
    void sendRemoveUserNotification(CGroupClient *connection, const QByteArray &name);
    void sendGroupTellMessage(const QVariantMap &root) override;
    void connectCommunicator() override;
    void disconnectCommunicator() override;
    void sendCharUpdate(const QVariantMap &map) override;
    void sendCharRename(const QVariantMap &map) override;
    bool kickCharacter(const QByteArray &) override;

private:
    void parseLoginInformation(CGroupClient *connection, const QVariantMap &data);
    void sendGroupInformation(CGroupClient *connection);
    void serverStartupFailed();

    QHash<QByteArray, CGroupClient *> clientsList{};
    CGroupServer server;
};

class CGroupClientCommunicator final : public CGroupCommunicator
{
    Q_OBJECT
    friend class CGroupClient;

public:
    explicit CGroupClientCommunicator(Mmapper2Group *parent);
    ~CGroupClientCommunicator() override;

public slots:
    void errorInConnection(CGroupClient *connection, const QString &);
    void retrieveData(CGroupClient *connection,
                      const Messages message,
                      const QVariantMap &data) override;
    void connectionClosed(CGroupClient *connection) override;

protected:
    void sendGroupTellMessage(const QVariantMap &map) override;
    void connectCommunicator() override;
    void disconnectCommunicator() override;
    void sendCharUpdate(const QVariantMap &map) override;
    void sendCharRename(const QVariantMap &map) override;
    bool kickCharacter(const QByteArray &) override;

private:
    void sendLoginInformation(CGroupClient *connection);

    CGroupClient client;
};

#endif /*CGROUPCOMMUNICATOR_H_*/
