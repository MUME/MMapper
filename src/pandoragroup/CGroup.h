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

#ifndef CGROUP_H_
#define CGROUP_H_

#include <QVector>
#include <QTreeWidget>
#include <QDomNode>

class MapData;
class CGroupCommunicator;
class CGroupChar;
class CGroupClient;

class CGroup : public QTreeWidget
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

    QByteArray getName() const ;
    CGroupChar* getCharByName(QByteArray name);

    void setType(int newState);
    int getType() const ;
    bool isConnected() const ;
    void reconnect() ;

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

    QDomNode getLocalCharData() ;
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

    void closeEvent( QCloseEvent * event ) ;
    void sendGTell(QByteArray tell); // sends gtell from local user
    void parseScoreInformation(QByteArray score);
    void parsePromptInformation(QByteArray prompt);
    void parseStateChangeLine(int message, QByteArray line);

  private:
    MapData* m_mapData;
};

#endif /*CGROUP_H_*/
