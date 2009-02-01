#ifndef CGROUPSERVER_H_
#define CGROUPSERVER_H_

#include <QList>
#include <QTcpServer>

#include "CGroupClient.h"
class CGroupCommunicator;

class CGroupServer: public QTcpServer
{
  Q_OBJECT

      CGroupCommunicator *getCommunicator() { return (CGroupCommunicator *) parent(); }
      QList<CGroupClient *> connections;
  public:
    CGroupServer(int localPort, QObject *parent);
    virtual ~CGroupServer();
    void addClient(CGroupClient *client);
    void sendToAll(QByteArray);
    void sendToAllExceptOne(CGroupClient *conn, QByteArray);
    void closeAll();

  protected:
    void incomingConnection(int socketDescriptor);

  public slots:
    void connectionClosed(CGroupClient *connection);

  private:


};

#endif /*CGROUPSERVER_H_*/
