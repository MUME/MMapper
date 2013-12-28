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

// draft for no peer
class CGroupDraftConnection : public QObject
{
  Q_OBJECT
  public:
    CGroupDraftConnection() {}
    virtual ~CGroupDraftConnection() {}
};

class CGroupCommunicator : public QObject
{
  Q_OBJECT
      int type;

  enum Messages { NONE, ACK,
    REQ_VERSION, REQ_ACK, REQ_LOGIN, REQ_INFO,
    PROT_VERSION, GTELL,
    STATE_LOGGED, STATE_KICKED,
    ADD_CHAR, REMOVE_CHAR, UPDATE_CHAR, RENAME_CHAR };

    QObject *peer;  // server or client
    CGroup *getGroup() { return reinterpret_cast<CGroup*>( parent() ); }

    void connectionClosed(CGroupClient *connection);
    void connectionEstablished(CGroupClient *connection);
    void connecting(CGroupClient *connection);
    QByteArray formMessageBlock(int message, QDomNode data);
    void sendMessage(CGroupClient *connection, int message, QByteArray data = "");
    void sendMessage(CGroupClient *connection, int message, QDomNode data);

    void sendLoginInformation(CGroupClient *connection);
    void parseLoginInformation(CGroupClient *connection, QDomNode data);
    void sendGroupInformation(CGroupClient *connection);
    void parseGroupInformation(CGroupClient *connection, QDomNode data);

    void userLoggedOn(CGroupClient *conn);
    void userLoggedOff(CGroupClient *conn);

    void retrieveDataClient(CGroupClient *connection, int message, QDomNode data);
    void retrieveDataServer(CGroupClient *connection, int message, QDomNode data);

    QHash<QByteArray, int>  clientsList;


public:
    const static int protocolVersion = 102;
    enum States { Server, Client, Off };


    CGroupCommunicator(int type, QObject *parent);
    virtual ~CGroupCommunicator();

    void changeType(int newState);
    int getType() {return type; }
    void sendCharUpdate(CGroupClient *conn, QDomNode blob);
    void sendCharUpdate(QDomNode blob);
    bool isConnected();
    void reconnect();
    void sendRemoveUserNotification(CGroupClient *conn, QByteArray name);
    void renameConnection(QByteArray oldName, QByteArray newName);
    void sendUpdateName(QByteArray oldName, QByteArray newName);

    void sendLog(const QString&);

  public slots:
    void connectionStateChanged(CGroupClient *connection);
    void errorInConnection(CGroupClient *connection, const QString&);
    void serverStartupFailed();
    void incomingData(CGroupClient *connection, QByteArray data);
    void sendGTell(QByteArray tell);
    void relayMessage(CGroupClient *connection, int message, QDomNode node);

  signals:
    void typeChanged(int type);

};

#endif /*CGROUPCOMMUNICATOR_H_*/
