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

#ifndef CONNECTIONLISTENER
#define CONNECTIONLISTENER

#include <QtGui>
#include <QtCore>
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
