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

#include "connectionlistener.h"
#include "proxy.h"

ConnectionListener::ConnectionListener(MapData* md, Mmapper2PathMachine* pm, CommandEvaluator* ce, PrespammedPath* pp, CGroup* gm, QObject *parent)
  : QTcpServer(parent)
{
  m_accept = true;

  m_mapData = md;
  m_pathMachine = pm;
  m_commandEvaluator = ce;
  m_prespammedPath = pp;
  m_groupManager = gm;

  connect(this, SIGNAL(log(const QString&, const QString&)), parent, SLOT(log(const QString&, const QString&)));
}


void ConnectionListener::incomingConnection(int socketDescriptor)
{
  if (m_accept)
  {
    emit log ("Listener", "New connection: accepted.");
    doNotAcceptNewConnections();
    Proxy *proxy = new Proxy(m_mapData, m_pathMachine, m_commandEvaluator, m_prespammedPath, m_groupManager, socketDescriptor, m_remoteHost, m_remotePort, true, this);
    proxy->start();
  }
  else
  {
    emit log ("Listener", "New connection: rejected.");
    QTcpSocket tcpSocket;
    if (tcpSocket.setSocketDescriptor(socketDescriptor)) {
      tcpSocket.write("You can't connect more than once!!!\r\n",37);
      tcpSocket.flush();
      tcpSocket.disconnectFromHost();
      tcpSocket.waitForDisconnected();
    }
  }
}

void ConnectionListener::doNotAcceptNewConnections()
{
  m_accept = false;
}

void ConnectionListener::doAcceptNewConnections()
{
  m_accept = true;
}


