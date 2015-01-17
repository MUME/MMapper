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

#ifndef MAPDATA_H
#define MAPDATA_H

#include "mapfrontend.h"
#include "abstractparser.h"

#include <QLinkedList>

class MapCanvas;
class InfoMark;
class RoomSelection;
class AbstractAction;

typedef QVector<Room*> RoomList;
typedef QVectorIterator<Room*> RoomListIterator;
typedef QLinkedList<InfoMark*> MarkerList;
typedef QLinkedListIterator<InfoMark*> MarkerListIterator;


class MapData : public MapFrontend
{

  Q_OBJECT
  friend class CustomAction;
public:
  MapData();
  virtual ~MapData();


  const RoomSelection * select(const Coordinate & ulf, const Coordinate & lrb);
  // updates a selection created by the mapdata
  const RoomSelection * select(const Coordinate & ulf, const Coordinate & lrb, const RoomSelection * in);
  // creates and registers a selection with one room
  const RoomSelection * select(const Coordinate & pos);
  // creates and registers an empty selection
  const RoomSelection * select();

  // selects the rooms given in "other" for "into"
  const RoomSelection * select(const RoomSelection * other, const RoomSelection * into);
  // removes the subset from the superset and unselects it
  void unselect(const RoomSelection * subset, const RoomSelection * superset);
  // removes the selection from the internal structures and deletes it
  void unselect(const RoomSelection * selection);
  // unselects a room from a selection
  void unselect(uint id, const RoomSelection * selection);

  // the room will be inserted in the given selection. the selection must have been created by mapdata
  const Room * getRoom(const Coordinate & pos, const RoomSelection * selection);
  const Room * getRoom(uint id, const RoomSelection * selection);

  void draw (const Coordinate & ulf, const Coordinate & lrb, MapCanvas & screen);
  bool isOccupied(const Coordinate & position);

  bool isMovable(const Coordinate & offset, const RoomSelection * selection);

  bool execute(MapAction * action);
  bool execute(MapAction * action, const RoomSelection * unlock);
  bool execute(AbstractAction * action, const RoomSelection * unlock);
    
  Coordinate & getPosition() {return m_position;}
  MarkerList& getMarkersList(){ return m_markers; };
  uint getRoomsCount(){ return greatestUsedId == UINT_MAX ? 0 : greatestUsedId+1; };
  int getMarkersCount(){ return m_markers.count(); };

  void addMarker(InfoMark* im);
  void removeMarker(InfoMark* im);

  bool isEmpty(){return (greatestUsedId == UINT_MAX && m_markers.isEmpty());};
  bool dataChanged(){return m_dataChanged;};
  QString getFileName(){ return m_fileName; };
  QList<Coordinate> getPath(const QList<CommandIdType> dirs);
  virtual void clear();

  // search for matches
  void searchDescriptions(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs);
  void searchNames(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs);
  void searchDoorNames(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs);
  void searchNotes(RoomRecipient * recipient, QString s, Qt::CaseSensitivity cs);

  // Used in Console Commands
  void removeDoorNames();
  QString getDoorName(const Coordinate & pos, uint dir);
  void setDoorName(const Coordinate & pos, const QString & name, uint dir);
  bool getExitFlag(const Coordinate & pos, uint flag, uint dir, uint field);
  void toggleExitFlag(const Coordinate & pos, uint flag, uint dir, uint field);
  void setRoomField(const Coordinate & pos, uint field, uint flag);
  uint getRoomField(const Coordinate & pos, uint flag);
  void toggleRoomFlag(const Coordinate & pos, uint flag, uint field);
  bool getRoomFlag(const Coordinate & pos, uint flag, uint field);

signals:
  void log( const QString&, const QString& );
  void onDataLoaded();
  void onDataChanged();

public slots:
  void setFileName(QString filename){ m_fileName = filename; };
  void unsetDataChanged(){m_dataChanged = false;};
  void setDataChanged(){m_dataChanged = true;};
  void setPosition(const Coordinate & pos) {m_position = pos;}

protected:
  std::map<const RoomSelection*, RoomSelection *> selections;

  MarkerList m_markers;

  // changed data?
  bool m_dataChanged;

  QString m_fileName;

  Coordinate m_position;
};


#endif
