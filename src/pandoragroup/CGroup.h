#ifndef CGROUP_H_
#define CGROUP_H_

#include <QDialog>
#include <QString>
#include <QDomNode>
#include <QVector>
//#include <QGridLayout>
#include <QCloseEvent>
//#include <QFrame>
#include <QHash>
#include <QTreeWidget>

#include "CGroupCommunicator.h"
#include "CGroupChar.h"
//#include "ui_groupmanager.h"

class MapData;

class CGroup : public QTreeWidget//, private Ui::GroupManager
{
  Q_OBJECT

  CGroupCommunicator *network;

  QVector<CGroupChar *> chars;
  CGroupChar      *self;
  //QFrame        *status;


  //QGridLayout *layout;

  void resetAllChars();

  signals:
    void log( const QString&, const QString& );
    void displayGroupTellEvent(const QByteArray& tell); // sends gtell from local user
    void drawCharacters(); // redraw the opengl screen

  public:

    enum StateMessages { NORMAL, FIGHTING, RESTING, SLEEPING, CASTING, INCAP, DEAD, BLIND, UNBLIND };

    CGroup(QByteArray name, MapData * md, QWidget *parent);
    virtual ~CGroup();

    QByteArray getName() { return self->getName(); }
    CGroupChar* getCharByName(QByteArray name);

    void setType(int newState);
    int getType() {return network->getType(); }
    bool isConnected() { return network->isConnected(); }
    void reconnect() { resetChars();  network->reconnect(); }

    bool addChar(QDomNode blob);
    void removeChar(QByteArray name);
    void removeChar(QDomNode node);
    bool isNamePresent(QByteArray name);
    QByteArray getNameFromBlob(QDomNode blob);
    void updateChar(QDomNode blob); // updates given char from the blob
    CGroupCommunicator *getCommunicator() { return network; }

    void resetChars();
    QVector<CGroupChar *>  getChars() { return chars; }
        // changing settings
    void resetName();
    void resetColor();

    QDomNode getLocalCharData() { return self->toXML(); }
    void sendAllCharsData(CGroupClient *conn);
    void issueLocalCharUpdate();

    void gTellArrived(QDomNode node);

        // dispatcher/Engine hooks
    bool isGroupTell(QByteArray tell);
    void renameChar(QDomNode blob);

    void sendLog(const QString&);

  public slots:
    void connectionRefused(QString message);
    void connectionFailed(QString message);
    void connectionClosed(QString message);
    void connectionError(QString message);
    void serverStartupFailed(QString message);
    void gotKicked(QDomNode message);
    void setCharPosition(unsigned int pos);

    void closeEvent( QCloseEvent * event ) {event->accept();}
    void sendGTell(QByteArray tell); // sends gtell from local user
    void parseScoreInformation(QByteArray score);
    void parsePromptInformation(QByteArray prompt);
    void parseStateChangeLine(int message, QByteArray line);

  private:
    MapData* m_mapData;
};

#endif /*CGROUP_H_*/
