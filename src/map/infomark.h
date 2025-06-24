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
    X(InfoMarkText, Text, ) \
    X(InfoMarkTypeEnum, Type, InfoMarkTypeEnum::TEXT) \
    X(InfoMarkClassEnum, Class, InfoMarkClassEnum::GENERIC) \
    X(Coordinate, Position1, ) \
    X(Coordinate, Position2, ) \
    X(int, RotationAngle, 0)

class InfoMark;

namespace tags {
struct NODISCARD InfomarkTextTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

using InfoMarkText = TaggedBoxedStringUtf8<tags::InfomarkTextTag>;

#define XFOREACH_INFOMARK_TYPE(X) X(TEXT) X(LINE) X(ARROW)

#define X_DECL(X) X,
enum class NODISCARD InfoMarkTypeEnum : uint8_t { XFOREACH_INFOMARK_TYPE(X_DECL) };
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
enum class NODISCARD InfoMarkClassEnum : uint8_t { XFOREACH_INFOMARK_CLASS(X_DECL) };
#undef X_DECL
#define X_COUNT(X) +1
static constexpr const size_t NUM_INFOMARK_CLASSES = XFOREACH_INFOMARK_CLASS(X_COUNT);
#undef X_COUNT
static_assert(NUM_INFOMARK_CLASSES == 10);

struct NODISCARD InfoMarkFields final
{
#define X_DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop{_OptInit};
    XFOREACH_INFOMARK_PROPERTY(X_DECL_FIELD)
#undef X_DECL_FIELD

public:
#define X_DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD inline const _Type &get##_Prop() const \
    { \
        return _Prop; \
    } \
    void set##_Prop(_Type value);
    XFOREACH_INFOMARK_PROPERTY(X_DECL_GETTERS_AND_SETTERS)
#undef X_DECL_GETTERS_AND_SETTERS

    void offsetBy(const Coordinate &offset)
    {
        setPosition1(getPosition1() + offset);
        setPosition2(getPosition2() + offset);
    }

    NODISCARD InfoMarkFields getOffsetCopy(const Coordinate &offset) const
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

struct NODISCARD InformarkChange final
{
    InfomarkId id = INVALID_INFOMARK_ID;
    InfoMarkFields fields{};
    explicit InformarkChange(InfomarkId i, InfoMarkFields f)
        : id(i)
        , fields(std::move(f))
    {}
};

template<>
struct std::hash<InfomarkId>
{
    std::size_t operator()(const InfomarkId &id) const noexcept { return numeric_hash(id.value()); }
};

struct NODISCARD ImmInfomarkIdSet final
{
private:
    using Set = ImmUnorderedSet<InfomarkId>;
    Set m_set;

public:
    ImmInfomarkIdSet() = default;

public:
    NODISCARD bool contains(InfomarkId id) const { return m_set.contains(id); }
    NODISCARD auto begin() const { return m_set.begin(); }
    NODISCARD auto end() const { return m_set.end(); }
    NODISCARD bool empty() const { return m_set.empty(); }
    NODISCARD auto size() const { return m_set.size(); }

public:
    void insert(InfomarkId id) { m_set.insert(id); }
    void remove(InfomarkId id) { m_set.erase(id); }

public:
    NODISCARD bool operator==(const ImmInfomarkIdSet &rhs) const { return m_set == rhs.m_set; }
    NODISCARD bool operator!=(const ImmInfomarkIdSet &rhs) const { return !(rhs == *this); }
};

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
    NODISCARD bool empty() const
    {
        return getIdSet().empty();
    }

public:
    NODISCARD InfomarkId addMarker(const InfoMarkFields &im);
    void updateMarker(InfomarkId, const InfoMarkFields &im);
    void updateMarkers(const std::vector<InformarkChange> &updates);
    NODISCARD InfoMarkFields getRawCopy(InfomarkId id) const;

    void removeMarker(InfomarkId);

public:
    NODISCARD InfomarkHandle find(InfomarkId) const;
    NODISCARD bool operator==(const InfomarkDb &rhs) const;
    NODISCARD bool operator!=(const InfomarkDb &rhs) const
    {
        return !(rhs == *this);
    }
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
    NODISCARD InfomarkId getId() const
    {
        return m_id;
    }
    NODISCARD bool exists() const
    {
        return m_id != INVALID_INFOMARK_ID;
    }
    NODISCARD explicit operator bool() const
    {
        return exists();
    }
    NODISCARD InfoMarkFields getRawCopy() const
    {
        return m_db.getRawCopy(m_id);
    }
};

using InfomarkPtr = std::optional<InfomarkHandle>;

NODISCARD extern InfoMarkText makeInfoMarkText(std::string text);
namespace mmqt {
NODISCARD extern InfoMarkText makeInfoMarkText(QString text);
}
