#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/Flags.h"
#include "../global/ImmUnorderedSet.h"
#include "../global/TaggedInt.h"
#include "../global/TaggedString.h"
#include "coordinate.h"

#include <cassert>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

static constexpr const auto INFOMARK_SCALE = 100;

#define XFOREACH_INFOMARK_PROPERTY(X) \
    X(InfomarkText, Text, ) \
    X(InfomarkTypeEnum, Type, InfomarkTypeEnum::TEXT) \
    X(InfomarkClassEnum, Class, InfomarkClassEnum::GENERIC) \
    X(Coordinate, Position1, ) \
    X(Coordinate, Position2, ) \
    X(int, RotationAngle, 0)

namespace tags {
struct NODISCARD InfomarkTextTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

using InfomarkText = TaggedBoxedStringUtf8<tags::InfomarkTextTag>;

#define XFOREACH_INFOMARK_TYPE(X) X(TEXT) X(LINE) X(ARROW)

#define X_DECL(X) X,
enum class NODISCARD InfomarkTypeEnum : uint8_t { XFOREACH_INFOMARK_TYPE(X_DECL) };
#undef X_DECL

#define X_COUNT(X) +1
static constexpr const size_t NUM_INFOMARK_TYPES = XFOREACH_INFOMARK_TYPE(X_COUNT);
#undef X_COUNT
static_assert(NUM_INFOMARK_TYPES == 3);

#define XFOREACH_INFOMARK_CLASS(X) \
    X(GENERIC) \
    X(HERB) \
    X(RIVER) \
    X(PLACE) \
    X(MOB) \
    X(COMMENT) \
    X(ROAD) \
    X(OBJECT) \
    X(ACTION) \
    X(LOCALITY)
#define X_DECL(X) X,
enum class NODISCARD InfomarkClassEnum : uint8_t { XFOREACH_INFOMARK_CLASS(X_DECL) };
#undef X_DECL
#define X_COUNT(X) +1
static constexpr const size_t NUM_INFOMARK_CLASSES = XFOREACH_INFOMARK_CLASS(X_COUNT);
#undef X_COUNT
static_assert(NUM_INFOMARK_CLASSES == 10);

struct NODISCARD RawInfomark final
{
#define X_DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop{_OptInit};
    XFOREACH_INFOMARK_PROPERTY(X_DECL_FIELD)
#undef X_DECL_FIELD

public:
#define X_DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD inline const _Type &get##_Prop() const { return _Prop; } \
    void set##_Prop(_Type value);
    XFOREACH_INFOMARK_PROPERTY(X_DECL_GETTERS_AND_SETTERS)
#undef X_DECL_GETTERS_AND_SETTERS

    void offsetBy(const Coordinate &offset)
    {
        setPosition1(getPosition1() + offset);
        setPosition2(getPosition2() + offset);
    }

    NODISCARD RawInfomark getOffsetCopy(const Coordinate &offset) const
    {
        auto copy = *this;
        copy.offsetBy(offset);
        return copy;
    }

    // Document the scaling system, even if nobody uses this function.
    // REVISIT: use a tag to differentiate the two coordinate types?
    NODISCARD static Coordinate worldToIM(const Coordinate &c)
    {
        return Coordinate{c.x * INFOMARK_SCALE, c.y * INFOMARK_SCALE, c.z};
    }
};

namespace tags {
struct NODISCARD InfomarkIdTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

struct NODISCARD InfomarkId final : public TaggedInt<InfomarkId, tags::InfomarkIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    InfomarkId() = default;
};

static constexpr const InfomarkId INVALID_INFOMARK_ID{UINT_MAX};

struct NODISCARD InfomarkChange final
{
    InfomarkId id = INVALID_INFOMARK_ID;
    RawInfomark mark{};
    explicit InfomarkChange(InfomarkId i, RawInfomark m)
        : id(i)
        , mark(std::move(m))
    {}
};

template<>
struct std::hash<InfomarkId>
{
    std::size_t operator()(const InfomarkId &id) const noexcept { return numeric_hash(id.value()); }
};

using ImmInfomarkIdSet = ImmUnorderedSet<InfomarkId>;

struct InfomarkHandle;
struct NODISCARD InfomarkDb final
{
private:
    struct Pimpl;
    std::shared_ptr<const Pimpl> m_pimpl;

public:
    InfomarkDb();

public:
#define X_DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD const _Type &get##_Prop(InfomarkId) const;
    XFOREACH_INFOMARK_PROPERTY(X_DECL_GETTERS_AND_SETTERS)
#undef X_DECL_GETTERS_AND_SETTERS

public:
    NODISCARD const ImmInfomarkIdSet &getIdSet() const;
    NODISCARD bool empty() const { return getIdSet().empty(); }

public:
    NODISCARD InfomarkId addMarker(const RawInfomark &im);
    void updateMarker(InfomarkId, const RawInfomark &im);
    void updateMarkers(const std::vector<InfomarkChange> &updates);
    NODISCARD RawInfomark getRawCopy(InfomarkId id) const;

    void removeMarker(InfomarkId);

public:
    NODISCARD InfomarkHandle find(InfomarkId) const;
    NODISCARD bool operator==(const InfomarkDb &rhs) const;
    NODISCARD bool operator!=(const InfomarkDb &rhs) const { return !(rhs == *this); }
};

struct NODISCARD InfomarkHandle final
{
private:
    InfomarkDb m_db;
    InfomarkId m_id;

public:
    explicit InfomarkHandle(InfomarkDb db, const InfomarkId id)
        : m_db(std::move(db))
        , m_id(id)
    {}

public:
#define X_DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD const _Type &get##_Prop() const;
    XFOREACH_INFOMARK_PROPERTY(X_DECL_GETTERS_AND_SETTERS)
#undef X_DECL_GETTERS_AND_SETTERS

public:
    NODISCARD InfomarkId getId() const { return m_id; }
    NODISCARD bool exists() const { return m_id != INVALID_INFOMARK_ID; }
    NODISCARD explicit operator bool() const { return exists(); }
    NODISCARD RawInfomark getRawCopy() const { return m_db.getRawCopy(m_id); }
};

using InfomarkPtr = std::optional<InfomarkHandle>;

NODISCARD extern InfomarkText makeInfomarkText(std::string text);
namespace mmqt {
NODISCARD extern InfomarkText makeInfomarkText(const QString &text);
}
