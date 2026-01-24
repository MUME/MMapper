#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/TaggedInt.h"
#include "../global/hash.h"
#include "../global/macros.h"

#include <cstdint>

namespace tags {
struct NODISCARD LocalSpaceIdTag final
{};
} // namespace tags

struct NODISCARD LocalSpaceId final : public TaggedInt<LocalSpaceId, tags::LocalSpaceIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    constexpr LocalSpaceId()
        : LocalSpaceId{0}
    {}
    NODISCARD constexpr uint32_t asUint32() const { return value(); }
};

static constexpr LocalSpaceId INVALID_LOCALSPACE_ID{0};

template<>
struct std::hash<LocalSpaceId>
{
    std::size_t operator()(const LocalSpaceId id) const noexcept
    {
        return numeric_hash(id.asUint32());
    }
};

struct NODISCARD LocalSpaceRenderData final
{
    float portalScale = 0.f;
    float portalX = 0.f;
    float portalY = 0.f;
    float portalZ = 0.f;
    float localCx = 0.f;
    float localCy = 0.f;
    float localCz = 0.f;
};
