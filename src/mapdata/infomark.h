/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara),
**            Anonymous <lachupe@gmail.com> (Azazello)
**
** This file is part of the MMapper project. 
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
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
#include <QtCore>

typedef class QString InfoMarkName;
typedef class QString InfoMarkText;
enum InfoMarkType { MT_TEXT, MT_LINE, MT_ARROW };

typedef QDateTime MarkerTimeStamp;

class InfoMark {

public:
    InfoMark(){};
    ~InfoMark(){};

    InfoMarkName getName(){ return m_name; };
    InfoMarkText getText(){ return m_text; };
    InfoMarkType getType() { return m_type; };
    Coordinate & getPosition1() {return m_pos1;}
    Coordinate & getPosition2() {return m_pos2;}
    MarkerTimeStamp getTimeStamp() { return m_timeStamp; };

    void setPosition1(Coordinate & pos) {m_pos1 = pos;}
    void setPosition2(Coordinate & pos) {m_pos2 = pos;}
    void setName(InfoMarkName name) { m_name = name; };
    void setText(InfoMarkText text) { m_text = text; };
    void setType(InfoMarkType type ){ m_type = type; };
    
    void setTimeStamp(MarkerTimeStamp timeStamp) { m_timeStamp = timeStamp; };

private:
    InfoMarkName m_name;
    InfoMarkType m_type;
    InfoMarkText m_text;
    
    MarkerTimeStamp m_timeStamp;

    Coordinate m_pos1;
    Coordinate m_pos2;
};

#endif
