#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "Crtp.h"

#include <array>

struct NODISCARD RoomFields final : public RoomFieldsGetters<RoomFields>,
                                    public RoomFieldsSetters<RoomFields>
{
public:
#define X_DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop{_OptInit};
    XFOREACH_ROOM_PROPERTY(X_DECL_FIELD)
#undef X_DECL_FIELD

public:
    NODISCARD bool operator==(const RoomFields &rhs) const
    {
#define X_CHECK(_Type, _Prop, _OptInit) \
    if ((_Prop) != (rhs._Prop)) { \
        return false; \
    }
        XFOREACH_ROOM_PROPERTY(X_CHECK)
#undef X_CHECK
        return true;
    }
    NODISCARD bool operator!=(const RoomFields &rhs) const
    {
        return !(rhs == *this);
    }

private:
    friend RoomFieldsGetters<RoomFields>;
    friend RoomFieldsSetters<RoomFields>;
    NODISCARD RoomFields &getRoomFields()
    {
        return *this;
    }
    NODISCARD const RoomFields &getRoomFields() const
    {
        return *this;
    }
};
