#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/ImmUnorderedMap.h"
#include "../global/macros.h"
#include "PromptFlags.h"
#include "RoomIdSet.h"
#include "mmapper2room.h"

#include <ostream>
#include <unordered_map>

class AnsiOstream;
class Map;
class ParseEvent;
class ProgressCounter;
class PathProcessor;

struct NODISCARD NameDesc final
{
    RoomName name;
    RoomDesc desc;
    NODISCARD bool operator==(const NameDesc &rhs) const
    {
        return name == rhs.name && desc == rhs.desc;
    }
    NODISCARD bool operator!=(const NameDesc &rhs) const { return !(rhs == *this); }
    NODISCARD bool operator<(const NameDesc &rhs) const
    {
        if (name < rhs.name) {
            return true;
        }
        if (rhs.name < name) {
            return false;
        }
        return desc < rhs.desc;
    }
    NODISCARD bool operator>(const NameDesc &rhs) const { return rhs < *this; }
    NODISCARD bool operator<=(const NameDesc &rhs) const { return !(rhs < *this); }
    NODISCARD bool operator>=(const NameDesc &rhs) const { return !(*this < rhs); }
};

template<>
struct std::hash<NameDesc>
{
    NODISCARD std::size_t operator()(const ::NameDesc &x) const noexcept
    {
        return std::hash<RoomName>()(x.name)
               ^ utils::rotate_bits64<32>(std::hash<RoomDesc>()(x.desc));
    }
};

enum class NODISCARD ParseKeyEnum { Name, Desc };
struct NODISCARD ParseKeyFlags final : public enums::Flags<ParseKeyFlags, ParseKeyEnum, uint8_t, 2>
{
    using Flags::Flags;
};

static constexpr const ParseKeyFlags ALL_PARSE_KEY_FLAGS = ~ParseKeyFlags{};
static_assert(ALL_PARSE_KEY_FLAGS == (ParseKeyFlags{ParseKeyEnum::Name} | ParseKeyEnum::Desc));

struct NODISCARD ParseTreeInitializer final
{
    std::unordered_map<RoomName, RoomIdSet> name_only;
    std::unordered_map<RoomDesc, RoomIdSet> desc_only;
    std::unordered_map<NameDesc, RoomIdSet> name_desc;
};

struct NODISCARD ParseTree final
{
    ImmUnorderedMap<RoomName, RoomIdSet> name_only;
    ImmUnorderedMap<RoomDesc, RoomIdSet> desc_only;
    ImmUnorderedMap<NameDesc, RoomIdSet> name_desc;

    void init(const ParseTreeInitializer &input)
    {
        name_only.init(input.name_only);
        desc_only.init(input.desc_only);
        name_desc.init(input.name_desc);
    }

    NODISCARD bool operator==(const ParseTree &rhs) const
    {
        return name_only == rhs.name_only && desc_only == rhs.desc_only
               && name_desc == rhs.name_desc;
    }
    NODISCARD bool operator!=(const ParseTree &rhs) const { return !(rhs == *this); }

    void printStats(ProgressCounter &pc, AnsiOstream &os) const;
};

NODISCARD RoomIdSet getRooms(const Map &map, const ParseTree &tree, const ParseEvent &event);
