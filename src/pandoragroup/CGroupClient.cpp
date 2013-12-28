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

#include <QHostAddress>
#include <QDebug>

#include "CGroupClient.h"
#include "CGroupCommunicator.h"

void CGroupClient::linkSignals()
{
  connect(this, SIGNAL(disconnected()), this, SLOT(lostConnection() ) );
  connect(this, SIGNAL(connected()), this, SLOT(connectionEstablished() ) );
  connect(this, SIGNAL(error(QAbstractSocket::SocketError )),
	  this, SLOT(errorHandler(QAbstractSocket::SocketError) ) );
  connect(this, SIGNAL(readyRead()), this, SLOT( dataIncoming() ) );

  buffer = "";
  currentMessageLen = 0;
}

CGroupClient::CGroupClient(QByteArray host, int remotePort, QObject *parent) :
  _host(host), _remotePort(remotePort), QTcpSocket(parent)
{
  linkSignals();
  setConnectionState(Connecting);
  protocolState = AwaitingLogin;
  connectToHost(host, remotePort);
}

CGroupClient::CGroupClient(QObject *parent) :
    QTcpSocket(parent)
{
  connectionState = Closed;
  protocolState = Idle;
  linkSignals();
}

void CGroupClient::setSocket(int socketDescriptor)
{
  if (setSocketDescriptor(socketDescriptor) == false) {
                // failure ... what to do?
    qDebug( "Connection failed. Native socket not recognized by CGroupClient.");
  }

  setConnectionState(Connected);
}

void CGroupClient::setProtocolState(int val)
{
//  qDebug( "Protocol state: %i", val);
  protocolState = val;
}

void CGroupClient::setConnectionState(int val)
{
//  qDebug( "Connection state: %i", val);
  connectionState = val;
  getParent()->connectionStateChanged(this);
}


CGroupClient::~CGroupClient()
{
//  printf("in CGroupClient destructor!\r\n");
}

void CGroupClient::lostConnection()
{
  setConnectionState(Closed);
}

void CGroupClient::connectionEstablished()
{
  setConnectionState(Connected);
}

void CGroupClient::errorHandler ( QAbstractSocket::SocketError socketError )
{
  CGroupCommunicator *comm = dynamic_cast<CGroupCommunicator *>( parent() );
  comm->errorInConnection(this, errorString() );
}

void CGroupClient::dataIncoming()
{
  QByteArray message;
  QByteArray rest;

//      qDebug( "Incoming Data [conn %i, IP: %s]", socketDescriptor(),
//                      (const char *) peerAddress().toString().toAscii() );

  QByteArray tmp = readAll();


  buffer += tmp;

//      qDebug( "RAW data buffer: %s", (const char *) buffer);

  while ( currentMessageLen < buffer.size()) {
//              qDebug( "in data-receiving cycle, buffer %s", (const char *) buffer);
    cutMessageFromBuffer();
  }
}

void CGroupClient::cutMessageFromBuffer()
{
  QByteArray rest;

  if (currentMessageLen == 0) {
    int index = buffer.indexOf(' ');

    QString len = buffer.left(index + 1);
    currentMessageLen = len.toInt();
//              qDebug( "Incoming buffer length: %i, incoming message length %i",
//                              buffer.size(), currentMessageLen);

    rest = buffer.right( buffer.size() - index - 1);
    buffer = rest;

    if (buffer.size() == currentMessageLen)
      cutMessageFromBuffer();

    //printf("returning from cutMessageFromBuffer\r\n");
    return;
  }

//      qDebug( "cutting off one message case");
  getParent()->incomingData(this, buffer.left(currentMessageLen));
  rest = buffer.right( buffer.size() - currentMessageLen);
  buffer = rest;
  currentMessageLen = 0;
}


void CGroupClient::sendData(QByteArray data)
{
  QByteArray buff;
  QString len;

  len = QString("%1 ").arg(data.size());

//      char len[10];

//      sprintf(len, "%i ", data.size());

  buff = len.toAscii();
  buff += data;

  write(buff);
}

CGroupCommunicator* CGroupClient::getParent() {
    return dynamic_cast<CGroupCommunicator *>( parent() );
}


