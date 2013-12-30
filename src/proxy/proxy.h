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

#ifndef PROXY
#define PROXY

//#define PROXY_STREAM_DEBUG_INPUT_TO_FILE

#include <QThread>
#include <QPointer>
#include <QTcpSocket>

class QFile;
class QDataStream;
class TelnetFilter;
class Parser;
class MumeXmlParser;
class Proxy;

class MapData;
class Mmapper2PathMachine;
class CommandEvaluator;
class PrespammedPath;
class CGroup;

class ProxyThreader: public QThread
{
  public:
    ProxyThreader(Proxy*);
    ~ProxyThreader();

    void run();

  protected:
    Q_OBJECT
        Proxy *m_proxy;
};


class Proxy : public QObject
{

  protected:
    Q_OBJECT

  public:
    Proxy(MapData*, Mmapper2PathMachine*, CommandEvaluator*, PrespammedPath*, CGroup*, int & socketDescriptor, QString & host, int & port, bool threaded, QObject *parent);
    ~Proxy();

    void start();

  public slots:
    void processMudStream();
    void processUserStream();
    void userTerminatedConnection();
    void mudTerminatedConnection();

    void sendToMud(const QByteArray&);
    void sendToUser(const QByteArray&);

  signals:
    void error(QTcpSocket::SocketError socketError);
    void log(const QString&, const QString&);
    void doNotAcceptNewConnections();
    void doAcceptNewConnections();

    void analyzeUserStream( const char *, int );
    void analyzeMudStream( const char *, int );
    void terminate();

  private:
    bool init();

#ifdef PROXY_STREAM_DEBUG_INPUT_TO_FILE
    QDataStream *debugStream;
    QFile* file;
#endif

    int m_socketDescriptor;
    QString m_remoteHost;
    int m_remotePort;
    QPointer<QTcpSocket> m_mudSocket, m_userSocket;
    char m_buffer[ 8192 ];

    bool m_serverConnected;

    TelnetFilter *m_filter;
    Parser *m_parser;
    MumeXmlParser *m_parserXml;

    MapData* m_mapData;
    Mmapper2PathMachine* m_pathMachine;
    CommandEvaluator* m_commandEvaluator;
    PrespammedPath* m_prespammedPath;
    CGroup* m_groupManager;

    ProxyThreader *m_thread;
    bool m_threaded;
    QObject *m_parent;
};

#endif

