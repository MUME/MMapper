#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/Array.h"
#include "../global/Badge.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/RoomHandle.h"
#include "../map/RoomRecipient.h"
#include "../map/coordinate.h"
#include "../map/roomid.h"

#include <memory>

class MapFrontend;

struct NODISCARD MouseSel final
{
    Coordinate2f pos;
    int layer = 0;

    MouseSel() = default;
    explicit MouseSel(const Coordinate2f &pos_, const int layer_)
        : pos{pos_}
        , layer{layer_}
    {}

    NODISCARD Coordinate getCoordinate() const { return Coordinate{pos.truncate(), layer}; }
    NODISCARD Coordinate getScaledCoordinate(const float xy_scale) const
    {
        return Coordinate{(pos * xy_scale).truncate(), layer};
    }

    NODISCARD glm::vec3 to_vec3() const
    {
        return glm::vec3{pos.to_vec2(), static_cast<float>(layer)};
    }
};

class NODISCARD ConnectionSelection final : public std::enable_shared_from_this<ConnectionSelection>
{
public:
    struct NODISCARD ConnectionDescriptor final
    {
        RoomPtr room = std::nullopt;
        ExitDirEnum direction = ExitDirEnum::NONE;

        NODISCARD static bool isTwoWay(const ConnectionDescriptor &first,
                                       const ConnectionDescriptor &second);
        NODISCARD static bool isOneWay(const ConnectionDescriptor &first,
                                       const ConnectionDescriptor &second);
        NODISCARD static bool isCompleteExisting(const ConnectionDescriptor &first,
                                                 const ConnectionDescriptor &second);
        NODISCARD static bool isCompleteNew(const ConnectionDescriptor &first,
                                            const ConnectionDescriptor &second);
    };

private:
    // REVISIT: give these enum names?
    MMapper::Array<ConnectionDescriptor, 2> m_connectionDescriptor;
    MapFrontend &m_map;
    bool m_first = true;

public:
    NODISCARD static std::shared_ptr<ConnectionSelection> alloc(MapFrontend &map,
                                                                const MouseSel &sel)
    {
        return std::make_shared<ConnectionSelection>(Badge<ConnectionSelection>{}, map, sel);
    }
    NODISCARD static std::shared_ptr<ConnectionSelection> alloc(MapFrontend &map)
    {
        return std::make_shared<ConnectionSelection>(Badge<ConnectionSelection>{}, map);
    }

    explicit ConnectionSelection(Badge<ConnectionSelection>, MapFrontend &map, const MouseSel &sel);
    explicit ConnectionSelection(Badge<ConnectionSelection>, MapFrontend &map);
    ~ConnectionSelection();
    DELETE_CTORS_AND_ASSIGN_OPS(ConnectionSelection);

    void setFirst(RoomId RoomId, ExitDirEnum dir);
    void setSecond(RoomId RoomId, ExitDirEnum dir);
    void setSecond(const MouseSel &sel);
    void removeSecond();

    NODISCARD ConnectionDescriptor getFirst() const;
    NODISCARD ConnectionDescriptor getSecond() const;

    // Valid just means the pointers aren't null.
    NODISCARD bool isValid() const;
    NODISCARD bool isFirstValid() const { return m_connectionDescriptor[0].room != std::nullopt; }
    NODISCARD bool isSecondValid() const { return m_connectionDescriptor[1].room != std::nullopt; }

    void receiveRoom(const RoomHandle &);

    // Complete means it actually describes a useful oneway or twoway exit.
    NODISCARD bool isCompleteExisting() const
    {
        return isValid() && ConnectionDescriptor::isCompleteExisting(getFirst(), getSecond());
    }
    NODISCARD bool isCompleteNew() const
    {
        return isValid() && ConnectionDescriptor::isCompleteNew(getFirst(), getSecond());
    }

private:
    NODISCARD static ExitDirEnum computeDirection(const Coordinate2f &mouse_f);
};
