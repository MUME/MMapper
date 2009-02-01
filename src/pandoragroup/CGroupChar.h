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
