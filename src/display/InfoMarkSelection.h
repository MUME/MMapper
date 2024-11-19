#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Badge.h"
#include "../map/infomark.h"
#include "../mapdata/mapdata.h"

#include <memory>

class NODISCARD InfoMarkSelection final : public MarkerList,
                                          public std::enable_shared_from_this<InfoMarkSelection>
{
private:
    Coordinate m_sel1;
    Coordinate m_sel2;

public:
    NODISCARD static std::shared_ptr<InfoMarkSelection> alloc(MapData &mapData,
                                                              const Coordinate &c1,
                                                              const Coordinate &c2)
    {
        return std::make_shared<InfoMarkSelection>(Badge<InfoMarkSelection>{}, mapData, c1, c2);
    }

public:
    InfoMarkSelection(Badge<InfoMarkSelection>,
                      MapData &,
                      const Coordinate &c1,
                      const Coordinate &c2);
    DELETE_CTORS_AND_ASSIGN_OPS(InfoMarkSelection);

    NODISCARD const Coordinate &getPosition1() const { return m_sel1; }
    NODISCARD const Coordinate &getPosition2() const { return m_sel2; }

    NODISCARD bool contains(InfoMark *im) const
    {
        const auto endIt = this->end();
        return std::find(begin(), endIt, im->shared_from_this()) != endIt;
    }
};
