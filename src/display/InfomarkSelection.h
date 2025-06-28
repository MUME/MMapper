#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Badge.h"
#include "../map/infomark.h"
#include "../mapdata/mapdata.h"

#include <memory>

class NODISCARD InfomarkSelection final : public std::enable_shared_from_this<InfomarkSelection>
{
private:
    MapData &m_mapData;
    MarkerList m_markerList;
    Coordinate m_sel1;
    Coordinate m_sel2;

public:
    NODISCARD static std::shared_ptr<InfomarkSelection> allocEmpty(MapData &mapData,
                                                                   const Coordinate &c1,
                                                                   const Coordinate &c2)
    {
        return std::make_shared<InfomarkSelection>(Badge<InfomarkSelection>{}, mapData, c1, c2);
    }
    NODISCARD static std::shared_ptr<InfomarkSelection> alloc(MapData &mapData,
                                                              const Coordinate &c1,
                                                              const Coordinate &c2)
    {
        auto result = allocEmpty(mapData, c1, c2);
        result->init();
        return result;
    }

public:
    InfomarkSelection(Badge<InfomarkSelection>,
                      MapData &,
                      const Coordinate &c1,
                      const Coordinate &c2);
    DELETE_CTORS_AND_ASSIGN_OPS(InfomarkSelection);

private:
    void init();

public:
    NODISCARD const Coordinate &getPosition1() const { return m_sel1; }
    NODISCARD const Coordinate &getPosition2() const { return m_sel2; }

public:
    NODISCARD size_t size() const { return m_markerList.size(); }
    NODISCARD bool empty() const { return m_markerList.empty(); }

public:
    NODISCARD const MarkerList &getMarkerList() const { return m_markerList; }

public:
    void emplace_back(InfomarkId id) { m_markerList.emplace_back(id); }
    void clear() { m_markerList.clear(); }

public:
    NODISCARD InfomarkHandle front() const
    {
        if (empty()) {
            throw std::runtime_error("InfomarkSelection is empty");
        }
        const auto &db = m_mapData.getCurrentMap().getInfomarkDb();
        return InfomarkHandle{db, m_markerList.front()};
    }

public:
    NODISCARD bool contains(InfomarkId im) const
    {
        const auto endIt = this->m_markerList.end();
        return std::find(m_markerList.begin(), endIt, im) != endIt;
    }

public:
    // Callback = void(const InfomarkHandle &)
    template<typename Callback>
    void for_each(Callback &&callback) const
    {
        static_assert(std::is_invocable_r_v<void, Callback, const InfomarkHandle &>);
        const auto &db = m_mapData.getCurrentMap().getInfomarkDb();
        for (const InfomarkId id : m_markerList) {
            const InfomarkHandle marker{db, id};
            callback(marker);
        }
    }

public:
    void applyOffset(const Coordinate &offset) const;
};
