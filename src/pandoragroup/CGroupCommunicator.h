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

#include <QObject>
#include <QDomNode>
#include <QHash>

class CGroup;
class CGroupClient;
class CGroupServer;
class GroupAction;

class CGroupCommunicator : public QObject
{
    Q_OBJECT
public:
    CGroupCommunicator(int type, QObject *parent);

    const static int protocolVersion = 102;

    enum Messages { NONE, ACK,
                    REQ_VERSION, REQ_ACK, REQ_LOGIN, REQ_INFO,
                    PROT_VERSION, GTELL,
                    STATE_LOGGED, STATE_KICKED,
                    ADD_CHAR, REMOVE_CHAR, UPDATE_CHAR, RENAME_CHAR
                  };

    int getType() const
    {
        return type;
    }
    void sendCharUpdate(CGroupClient *, const QDomNode &);
    void sendMessage(CGroupClient *, int, const QByteArray &blob = "");
    void sendMessage(CGroupClient *, int, const QDomNode &);
    void renameConnection(const QByteArray &, const QByteArray &);

    virtual void disconnect() = 0;
    virtual void reconnect() = 0;
    virtual void sendGroupTellMessage(QDomElement) = 0;
    virtual void sendCharUpdate(QDomNode) = 0;
    virtual void sendCharRename(QDomNode) = 0;

protected:
    QByteArray formMessageBlock(int message, const QDomNode &data);
    CGroup *getGroup();

public slots:
    void incomingData(CGroupClient *, const QByteArray &);
    void sendGTell(const QByteArray &);
    void relayLog(const QString &);
    virtual void connectionEstablished(CGroupClient *) {}
    virtual void retrieveData(CGroupClient *, int, QDomNode) = 0;
    virtual void connectionClosed(CGroupClient *) = 0;

signals:
    void networkDown();
    void messageBox(QString message);
    void scheduleAction(GroupAction *action);
    void gTellArrived(QDomNode node);
    void sendLog(const QString &);

private:
    int type;
};

class CGroupServerCommunicator : public CGroupCommunicator
{
    Q_OBJECT
    friend class CGroupServer;
public:
    CGroupServerCommunicator(QObject *parent);
    ~CGroupServerCommunicator();

    void renameConnection(const QByteArray &oldName, const QByteArray &newName);

protected slots:
    void relayMessage(CGroupClient *connection, int message, const QDomNode &data);
    void serverStartupFailed();
    void connectionEstablished(CGroupClient *);
    void retrieveData(CGroupClient *connection, int message, QDomNode data);
    void connectionClosed(CGroupClient *connection);

protected:
    void sendRemoveUserNotification(CGroupClient *connection, const QByteArray &name);
    void sendGroupTellMessage(QDomElement root);
    void reconnect();
    void disconnect();
    void sendCharUpdate(QDomNode blob);
    void sendCharRename(QDomNode blob);

private:
    void parseLoginInformation(CGroupClient *connection, const QDomNode &data);
    void sendGroupInformation(CGroupClient *connection);

    QHash<QByteArray, int>  clientsList;
    CGroupServer *server;

};

class CGroupClientCommunicator : public CGroupCommunicator
{
    Q_OBJECT
    friend class CGroupClient;
public:
    CGroupClientCommunicator(QObject *parent);
    ~CGroupClientCommunicator();

public slots:
    void errorInConnection(CGroupClient *connection, const QString &);
    void retrieveData(CGroupClient *conn, int message, QDomNode data);
    void connectionClosed(CGroupClient *connection);

protected:
    void sendGroupTellMessage(QDomElement root);
    void reconnect();
    void disconnect();
    void sendCharUpdate(QDomNode blob);
    void sendCharRename(QDomNode blob);

private:
    void sendLoginInformation(CGroupClient *connection);

    CGroupClient *client;

};

#endif /*CGROUPCOMMUNICATOR_H_*/
