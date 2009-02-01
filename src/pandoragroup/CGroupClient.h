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
