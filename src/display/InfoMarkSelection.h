#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>

#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"

class InfoMarkSelection final : public MarkerList,
                                public std::enable_shared_from_this<InfoMarkSelection>
{
private:
    struct this_is_private final
    {
        explicit this_is_private(int) {}
    };

public:
    static std::shared_ptr<InfoMarkSelection> alloc(MapData *mapData,
                                                    const Coordinate &c1,
                                                    const Coordinate &c2,
                                                    int margin = 20)
    {
        return std::make_shared<InfoMarkSelection>(this_is_private{0}, mapData, c1, c2, margin);
    }

    static std::shared_ptr<InfoMarkSelection> alloc(MapData *mapData, const Coordinate &c)
    {
        return std::make_shared<InfoMarkSelection>(this_is_private{0}, mapData, c, c, 100);
    }

public:
    InfoMarkSelection(
        this_is_private, MapData *, const Coordinate &c1, const Coordinate &c2, int margin);
    DELETE_CTORS_AND_ASSIGN_OPS(InfoMarkSelection);

    const Coordinate &getPosition1() const { return m_sel1; }
    const Coordinate &getPosition2() const { return m_sel2; }

    bool contains(InfoMark *im) const
    {
        const auto endIt = this->end();
        return std::find(begin(), endIt, im->shared_from_this()) != endIt;
    }

private:
    Coordinate m_sel1;
    Coordinate m_sel2;
};
