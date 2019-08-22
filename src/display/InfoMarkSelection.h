#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#include "../mapdata/mapdata.h"

class InfoMarkSelection final : public MarkerList
{
public:
    explicit InfoMarkSelection(MapData *,
                               const Coordinate &c1,
                               const Coordinate &c2,
                               const int margin = 20);
    explicit InfoMarkSelection(MapData *, const Coordinate &c);

    const Coordinate &getPosition1() const { return m_sel1; }
    const Coordinate &getPosition2() const { return m_sel2; }

private:
    Coordinate m_sel1{}, m_sel2{};
};
