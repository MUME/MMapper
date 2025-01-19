#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/Flags.h"
#include "../global/TaggedString.h"
#include "coordinate.h"

#include <cassert>
#include <memory>

static constexpr const auto INFOMARK_SCALE = 100;

enum class NODISCARD InfoMarkUpdateEnum {
    InfoMarkText,
    InfoMarkType,
    InfoMarkClass,
    CoordinatePosition1,
    CoordinatePosition2,
    RotationAngle
};

static constexpr const size_t NUM_INFOMARK_UPDATE_TYPES = 6;
static_assert(NUM_INFOMARK_UPDATE_TYPES == static_cast<int>(InfoMarkUpdateEnum::RotationAngle) + 1);
DEFINE_ENUM_COUNT(InfoMarkUpdateEnum, NUM_INFOMARK_UPDATE_TYPES)

struct NODISCARD InfoMarkUpdateFlags final
    : enums::Flags<InfoMarkUpdateFlags, InfoMarkUpdateEnum, uint32_t>
{
    using Flags::Flags;
};

class InfoMark;

class NODISCARD InfoMarkModificationTracker
{
public:
    virtual ~InfoMarkModificationTracker();

public:
    void notifyModified(InfoMark &mark, InfoMarkUpdateFlags updateFlags);
    virtual void virt_onNotifyModified(InfoMark & /*mark*/, InfoMarkUpdateFlags /*updateFlags*/) {}
};

struct NODISCARD InfomarkTextTag
{};

using InfoMarkText = TaggedStringLatin1<InfomarkTextTag>;

#define X_FOREACH_INFOMARK_TYPE(X) \
    X(TEXT) \
    X(LINE) \
    X(ARROW)

#define DECL(X) X,
enum class NODISCARD InfoMarkTypeEnum : uint8_t { X_FOREACH_INFOMARK_TYPE(DECL) };
#undef DECL
#define ADD(X) +1
static constexpr const size_t NUM_INFOMARK_TYPES = (X_FOREACH_INFOMARK_TYPE(ADD));
#undef ADD
static_assert(NUM_INFOMARK_TYPES == 3);
DEFINE_ENUM_COUNT(InfoMarkTypeEnum, NUM_INFOMARK_TYPES)

#define X_FOREACH_INFOMARK_CLASS(X) \
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

#define DECL(X) X,
enum class NODISCARD InfoMarkClassEnum : uint8_t { X_FOREACH_INFOMARK_CLASS(DECL) };
#undef DECL
#define ADD(X) +1
static constexpr const size_t NUM_INFOMARK_CLASSES = (X_FOREACH_INFOMARK_CLASS(ADD));
#undef ADD
static_assert(NUM_INFOMARK_CLASSES == 10);
DEFINE_ENUM_COUNT(InfoMarkClassEnum, NUM_INFOMARK_CLASSES)

#define X_FOREACH_INFOMARK_PROPERTY(X) \
    X(InfoMarkText, Text, ) \
    X(InfoMarkTypeEnum, Type, = InfoMarkTypeEnum::TEXT) \
    X(InfoMarkClassEnum, Class, = InfoMarkClassEnum::GENERIC) \
    X(Coordinate, Position1, ) \
    X(Coordinate, Position2, ) \
    X(int, RotationAngle, = 0)

using SharedInfoMark = std::shared_ptr<InfoMark>;

class NODISCARD InfoMark final : public std::enable_shared_from_this<InfoMark>
{
private:
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };
    struct NODISCARD InfoMarkFields final
    {
#define DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop _OptInit;
        X_FOREACH_INFOMARK_PROPERTY(DECL_FIELD)
#undef DECL_FIELD
    };

public:
    NODISCARD static std::shared_ptr<InfoMark> alloc(InfoMarkModificationTracker &tracker)
    {
        return std::make_shared<InfoMark>(this_is_private{0}, tracker);
    }

public:
#define DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD inline const _Type &get##_Prop() const \
    { \
        return m_fields._Prop; \
    } \
    void set##_Prop(_Type value);
    X_FOREACH_INFOMARK_PROPERTY(DECL_GETTERS_AND_SETTERS)
#undef DECL_GETTERS_AND_SETTERS

public:
    InfoMark() = delete;
    explicit InfoMark(this_is_private, InfoMarkModificationTracker &tracker);
    ~InfoMark() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(InfoMark);

public:
    void setModified(InfoMarkUpdateFlags updateFlags);

private:
    InfoMarkModificationTracker &m_tracker;
    InfoMarkFields m_fields;
};
