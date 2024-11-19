#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "ChangeTypes.h"
#include "roomid.h"

#include <optional>
#include <variant>

#include <qmetatype.h>

class NODISCARD Change final
{
private:
#define X_DECL(_Type) _Type
#define X_COMMA() ,
    using Variant = std::variant<XFOREACH_CHANGE_TYPE(X_DECL, X_COMMA)>;
#undef X_DECL
#undef X_COMMA
    Variant m_variant;

public:
#define X_NOP()
#define X_DECL_CTOR(_Type) \
    explicit Change(_Type x) \
        : m_variant{std::move(x)} \
    {}
    XFOREACH_CHANGE_TYPE(X_DECL_CTOR, X_NOP)
#undef X_DECL_CTOR
#undef X_NOP

    template<typename Visitor>
    void acceptVisitor(Visitor &&visitor) const
    {
        std::visit(std::forward<Visitor>(visitor), m_variant);
    }

    NODISCARD std::optional<RoomId> isRemoveRoom() const
    {
        if (std::holds_alternative<room_change_types::RemoveRoom>(m_variant)) {
            return std::get<room_change_types::RemoveRoom>(m_variant).room;
        }
        return std::nullopt;
    }
};
