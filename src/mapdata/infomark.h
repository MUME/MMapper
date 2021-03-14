#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cassert>
#include <memory>

#include "../expandoracommon/coordinate.h"
#include "../global/Flags.h"
#include "../global/TaggedString.h"

static constexpr const auto INFOMARK_SCALE = 100;

enum class InfoMarkUpdateEnum {
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

struct InfoMarkUpdateFlags final : enums::Flags<InfoMarkUpdateFlags, InfoMarkUpdateEnum, uint32_t>
{
    using Flags::Flags;
};

class InfoMark;

class InfoMarkModificationTracker
{
public:
    virtual ~InfoMarkModificationTracker();

public:
    void notifyModified(InfoMark &mark, InfoMarkUpdateFlags updateFlags);
    virtual void virt_onNotifyModified(InfoMark & /*mark*/, InfoMarkUpdateFlags /*updateFlags*/) {}
};

struct InfomarkTextTag
{};

using InfoMarkText = TaggedString<InfomarkTextTag>;

enum class InfoMarkTypeEnum { TEXT, LINE, ARROW };
static constexpr const size_t NUM_INFOMARK_TYPES = static_cast<size_t>(InfoMarkTypeEnum::ARROW)
                                                   + 1u;
static_assert(NUM_INFOMARK_TYPES == 3);
enum class InfoMarkClassEnum {
    GENERIC,
    HERB,
    RIVER,
    PLACE,
    MOB,
    COMMENT,
    ROAD,
    OBJECT,
    ACTION,
    LOCALITY
};
static constexpr const size_t NUM_INFOMARK_CLASSES = static_cast<size_t>(InfoMarkClassEnum::LOCALITY)
                                                     + 1u;
static_assert(NUM_INFOMARK_CLASSES == 10);

#define XFOREACH_INFOMARK_PROPERTY(X) \
    X(InfoMarkText, Text, ) \
    X(InfoMarkTypeEnum, Type, = InfoMarkTypeEnum::TEXT) \
    X(InfoMarkClassEnum, Class, = InfoMarkClassEnum::GENERIC) \
    X(Coordinate, Position1, ) \
    X(Coordinate, Position2, ) \
    X(int, RotationAngle, = 0)

class InfoMark final : public std::enable_shared_from_this<InfoMark>
{
private:
    struct this_is_private final
    {
        explicit this_is_private(int) {}
    };
    struct InfoMarkFields final
    {
#define DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop _OptInit;
        XFOREACH_INFOMARK_PROPERTY(DECL_FIELD)
#undef DECL_FIELD
    };

public:
    static std::shared_ptr<InfoMark> alloc(InfoMarkModificationTracker &tracker)
    {
        return std::make_shared<InfoMark>(this_is_private{0}, tracker);
    }

public:
#define DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    inline const _Type &get##_Prop() const { return m_fields._Prop; } \
    void set##_Prop(_Type value);
    XFOREACH_INFOMARK_PROPERTY(DECL_GETTERS_AND_SETTERS)
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
