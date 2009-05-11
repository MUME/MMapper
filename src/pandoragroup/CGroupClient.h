/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara),
**            Anonymous <lachupe@gmail.com> (Azazello)
**
** This file is part of the MMapper project. 
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
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

/*
 *  Pandora MUME mapper
 *
 *  Copyright (C) 2000-2009  Azazello
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CGROUPCLIENT_H_
#define CGROUPCLIENT_H_

#include <QTcpSocket>
#include "CGroupCommunicator.h"


class CGroupClient : public QTcpSocket
{
  Q_OBJECT

      int      connectionState;
  int  protocolState;

  CGroupCommunicator* getParent() { return (CGroupCommunicator *) parent(); }
  void linkSignals();

  QByteArray buffer;
  int currentMessageLen;

  void cutMessageFromBuffer();
  public:
    enum ConnectionStates { Closed, Connecting, Connected, Quiting};
    enum ProtocolStates { Idle, AwaitingLogin, AwaitingInfo, Logged };

    CGroupClient(QObject *parent);
    CGroupClient(QByteArray host, int remotePort, QObject *parent);
    virtual ~CGroupClient();

    void setSocket(int socketDescriptor);

    int getConnectionState() {return connectionState; }
    void setConnectionState(int val);
    void setProtocolState(int val);
    int getProtocolState() { return protocolState; }
    void sendData(QByteArray data);

  protected slots:
    void lostConnection();
    void connectionEstablished();
    void errorHandler ( QAbstractSocket::SocketError socketError );
    void dataIncoming();

  signals:

};

#endif /*CGROUPCLIENT_H_*/
