/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
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

#ifndef CONNECTIONLISTENER
#define CONNECTIONLISTENER

#include <QTcpServer>
#include <QTcpSocket>

class MapData;
class Mmapper2PathMachine;
class CommandEvaluator;
class PrespammedPath;
class CGroup;

class ConnectionListener : public QTcpServer {
  public:
    ConnectionListener(MapData*, Mmapper2PathMachine*, CommandEvaluator*, PrespammedPath*, CGroup*, QObject *parent);

    QString getRemoteHost() const {return m_remoteHost;}
    void setRemoteHost(QString i) {m_remoteHost = i;}
    int getRemotePort() const {return m_remotePort;}
    void setRemotePort(int i) {m_remotePort = i;}

  public slots:
    void doNotAcceptNewConnections();
    void doAcceptNewConnections();

  signals:
    void log(const QString&, const QString&);

  protected:
    void incomingConnection(int socketDescriptor);

  private:
    Q_OBJECT

        QString m_remoteHost;
    int m_remotePort;

    MapData* m_mapData;
    Mmapper2PathMachine* m_pathMachine;
    CommandEvaluator* m_commandEvaluator;
    PrespammedPath* m_prespammedPath;
    CGroup* m_groupManager;

        bool m_accept;
};

#endif
