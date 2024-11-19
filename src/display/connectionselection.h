#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h" /* Coordinate2f */
#include "../global/Array.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

#include <memory>

class MapFrontend;
class Room;
class RoomAdmin;
struct RoomId;

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

class NODISCARD ConnectionSelection final : public RoomRecipient,
                                            public std::enable_shared_from_this<ConnectionSelection>
{
public:
    struct NODISCARD ConnectionDescriptor final
    {
        const Room *room = nullptr;
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
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };

private:
    // REVISIT: give these enum names?
    MMapper::Array<ConnectionDescriptor, 2> m_connectionDescriptor;

    bool m_first = true;
    RoomAdmin *m_admin = nullptr;

public:
    NODISCARD static std::shared_ptr<ConnectionSelection> alloc(MapFrontend *mf, const MouseSel &sel)
    {
        return std::make_shared<ConnectionSelection>(this_is_private{0}, mf, sel);
    }
    NODISCARD static std::shared_ptr<ConnectionSelection> alloc()
    {
        return std::make_shared<ConnectionSelection>(this_is_private{0});
    }

    explicit ConnectionSelection(this_is_private, MapFrontend *mf, const MouseSel &sel);
    explicit ConnectionSelection(this_is_private);
    ~ConnectionSelection() override;
    DELETE_CTORS_AND_ASSIGN_OPS(ConnectionSelection);

    void setFirst(MapFrontend *mf, RoomId RoomId, ExitDirEnum dir);
    void setSecond(MapFrontend *mf, RoomId RoomId, ExitDirEnum dir);
    void setSecond(MapFrontend *mf, const MouseSel &sel);
    void removeSecond();

    NODISCARD ConnectionDescriptor getFirst() const;
    NODISCARD ConnectionDescriptor getSecond() const;

    // Valid just means the pointers aren't null.
    bool isValid() const;
    bool isFirstValid() const { return m_connectionDescriptor[0].room != nullptr; }
    bool isSecondValid() const { return m_connectionDescriptor[1].room != nullptr; }

private:
    void virt_receiveRoom(RoomAdmin *admin, const Room *aRoom) final;

public:
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
