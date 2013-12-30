/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef INFOMARK_H
#define INFOMARK_H

#include "coordinate.h"
#include <QDateTime>

typedef class QString InfoMarkName;
typedef class QString InfoMarkText;
enum InfoMarkType { MT_TEXT, MT_LINE, MT_ARROW };

typedef QDateTime MarkerTimeStamp;

class InfoMark {

public:
    InfoMark(){ m_type = MT_TEXT; }
    ~InfoMark(){};

    const InfoMarkName& getName() const { return m_name; }
    const InfoMarkText& getText() const { return m_text; }
    InfoMarkType getType() const { return m_type; }
    const Coordinate& getPosition1() const {return m_pos1;}
    const Coordinate& getPosition2() const {return m_pos2;}
    const MarkerTimeStamp& getTimeStamp() const { return m_timeStamp; }

    void setPosition1(Coordinate & pos) {m_pos1 = pos;}
    void setPosition2(Coordinate & pos) {m_pos2 = pos;}
    void setName(InfoMarkName name) { m_name = name; }
    void setText(InfoMarkText text) { m_text = text; }
    void setType(InfoMarkType type ){ m_type = type; }
    
    void setTimeStamp(MarkerTimeStamp timeStamp) { m_timeStamp = timeStamp; }

private:
    InfoMarkName m_name;
    InfoMarkType m_type;
    InfoMarkText m_text;
    
    MarkerTimeStamp m_timeStamp;

    Coordinate m_pos1;
    Coordinate m_pos2;
};

#endif
