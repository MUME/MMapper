/*
 *  Pandora MUME mapper
 *
 *  Copyright (C) 2000-2009  Azazello
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CGROUPCHAR_H_
#define CGROUPCHAR_H_

#include <QByteArray>
#include <QPixmap>
#include <QDomNode>
#include <QLabel>
#include <QGridLayout>
#include <QFrame>
#include "mapdata.h"

class CGroupChar
{
  unsigned int pos;
  QByteArray name;
  QByteArray textHP;
  QByteArray textMoves;
  QByteArray textMana;
  QByteArray lastMovement;
  int hp, maxhp;
  int mana, maxmana;
  int moves, maxmoves;
  int state;
  QColor color;
  QPixmap pixmap;

  QTreeWidgetItem *charItem;
  public:
    enum Char_States { NORMAL, BASHED, INCAPACITATED, DEAD };
    CGroupChar(MapData*, QTreeWidget*);
    virtual ~CGroupChar();

    QByteArray getName() { return name; }
    void setName(QByteArray _name) { name = _name; }
    void setColor(QColor col) { color = col; updateLabels(); }
    QColor getColor() { return color; }
    QDomNode toXML();
    bool updateFromXML(QDomNode blob);
    QTreeWidgetItem *getCharItem() { return charItem; }

    void setLastMovement(QByteArray move) { lastMovement = move; }
    void setPosition(unsigned int id) { pos = id; }
    unsigned int getPosition() { return pos; }
    QByteArray getLastMovement() { return lastMovement; }
    static QByteArray getNameFromXML(QDomNode node);

    void draw(int x, int y);
    void updateLabels();

    // for local char only
    void setScore(int _hp, int _maxhp, int _mana, int _maxmana, int _moves, int _maxmoves)
    {
      hp = _hp; maxhp = _maxhp; mana = _mana; maxmana = _maxmana;
      moves = _moves; maxmoves = _maxmoves;
    }

    void setTextScore(QByteArray hp, QByteArray mana, QByteArray moves)
    {
      textHP = hp; textMana = mana; textMoves = moves;
    }

  private:
    MapData* m_mapData;
    QTreeWidget* charTable;

    void setItemText(uint itemNumber, const QString&);

};

#endif /*CGROUPCHAR_H_*/
