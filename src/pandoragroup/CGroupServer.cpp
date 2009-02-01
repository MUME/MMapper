#include <QHostAddress>

#include "CGroupServer.h"

CGroupServer::CGroupServer(int localPort, QObject *parent) :
        QTcpServer(parent)
{
  printf("Creating the GroupManager Server.\r\n");
  if (listen(QHostAddress::Any, localPort) != true) {
    getCommunicator()->sendLog("Failed to start a group Manager server!");
    getCommunicator()->serverStartupFailed();
  } else {
    getCommunicator()->sendLog(QString("Listening on port %1!").arg(localPort));
  }
}

CGroupServer::~CGroupServer()
{

}

void CGroupServer::incomingConnection(int socketDescriptor)
{
  getCommunicator()->sendLog("Incoming connection");
        // connect the client straight to the Communicator, as he handles all the state changes
        // data transfers and similar.
  CGroupClient *client = new CGroupClient(parent());
  addClient(client);

  client->setSocket(socketDescriptor);
}

void CGroupServer::addClient(CGroupClient *client)
{
  getCommunicator()->sendLog("Adding a client to the connections list.");
  connections.append(client);
}

void CGroupServer::connectionClosed(CGroupClient *connection)
{
  getCommunicator()->sendLog("Removing and deleting the connection completely.");
  connections.removeAll(connection);
  getCommunicator()->sendLog("Deleting the connection");
  connection->deleteLater();
}

void CGroupServer::sendToAll(QByteArray message)
{
  sendToAllExceptOne(NULL, message);
}


void CGroupServer::sendToAllExceptOne(CGroupClient *conn, QByteArray message)
{
  for (int i = 0; i < connections.size(); i++)
    if (connections[i] != conn)
      connections[i]->sendData(message);
}

void CGroupServer::closeAll()
{

  printf("Closing connections\r\n");
  for (int i = 0; i < connections.size(); i++) {
    printf("Closing connections %i\r\n", connections[i]->socketDescriptor());
    connections[i]->deleteLater();
  }
}

