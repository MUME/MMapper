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

#ifndef DOOR_H
#define DOOR_H

#include <QString>

#define DF_HIDDEN     bit1
#define DF_NEEDKEY    bit2
#define DF_NOBLOCK    bit3
#define DF_NOBREAK    bit4
#define DF_NOPICK     bit5
#define DF_DELAYED    bit6
#define DF_RESERVED1  bit7
#define DF_RESERVED2  bit8
typedef class QString DoorName;
typedef quint8 DoorFlags;

class Door {

public:
    Door (DoorName name = "", DoorFlags flags = 0){ m_name = name; m_flags = flags; }
    DoorFlags getFlags() const { return m_flags; };
    const DoorName& getName() const { return m_name; };
    void setFlags(DoorFlags flags) { m_flags = flags; };
    void setName(const DoorName & name) { m_name = name; };

private:
    DoorName  m_name;
    DoorFlags m_flags;
};

#endif
