#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    Coordinate m_sel1;
    Coordinate m_sel2;
};
