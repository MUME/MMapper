/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project.
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef PROXY
#define PROXY

//#define PROXY_STREAM_DEBUG_INPUT_TO_FILE

#include <QtGui>
#include <QtCore>
#include <QtNetwork>

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
    QTcpSocket *m_mudSocket;
    QTcpSocket *m_userSocket;
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

