#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"
#include "../global/TaggedInt.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>

// TODO: Comm

// X(UPPER_CASE, CamelCase, "normalized name", "friendly name")
#define XFOREACH_GMCP_MODULE_TYPE(X) \
    X(CHAR, Char, "char", "Char") \
    X(EVENT, Event, "event", "Event") \
    X(GROUP, Group, "group", "Group") \
    X(EXTERNAL_DISCORD, ExternalDiscord, "external.discord", "External.Discord") \
    X(MUME_CLIENT, MumeClient, "mume.client", "MUME.Client") \
    X(MUME_TIME, MumeTime, "mume.time", "MUME.Time") \
    X(ROOM_CHARS, RoomChars, "room.chars", "Room.Chars") \
    X(ROOM, Room, "room", "Room") \
    /* define gmcp module types above */

enum class NODISCARD GmcpModuleTypeEnum {
    UNKNOWN = -1,
#define X_DECL_GMCP_MODULE_TYPE(UPPER_CASE, CamelCase, normalized, friendly) UPPER_CASE,
    XFOREACH_GMCP_MODULE_TYPE(X_DECL_GMCP_MODULE_TYPE)
#undef X_DECL_GMCP_MODULE_TYPE
};

#define X_COUNT(...) +1
static constexpr const size_t NUM_GMCP_MODULES = XFOREACH_GMCP_MODULE_TYPE(X_COUNT);
#undef X_COUNT
static_assert(NUM_GMCP_MODULES == 8);
DEFINE_ENUM_COUNT(GmcpModuleTypeEnum, NUM_GMCP_MODULES)

namespace tags {
struct NODISCARD TagGmcpModuleVersionTag final
{};
} // namespace tags

struct NODISCARD GmcpModuleVersion final
    : public TaggedInt<GmcpModuleVersion, tags::TagGmcpModuleVersionTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    NODISCARD uint32_t asUint32() const { return value(); }
};

static constexpr const GmcpModuleVersion DEFAULT_GMCP_MODULE_VERSION{0};

using GmcpModuleVersionList
    = EnumIndexedArray<GmcpModuleVersion, GmcpModuleTypeEnum, NUM_GMCP_MODULES>;

class NODISCARD GmcpModule final
{
private:
    struct NODISCARD NameVersion final
    {
        std::string normalizedName; // utf8
        GmcpModuleVersion version = DEFAULT_GMCP_MODULE_VERSION;

        NODISCARD static NameVersion fromStdString(std::string);
    };

    NameVersion m_nameVersion;
    GmcpModuleTypeEnum m_type = GmcpModuleTypeEnum::UNKNOWN;

public:
    explicit GmcpModule(std::string);
    explicit GmcpModule(const std::string &, GmcpModuleVersion);
    explicit GmcpModule(GmcpModuleTypeEnum, GmcpModuleVersion);
    DEFAULT_RULE_OF_5(GmcpModule);

public:
    NODISCARD bool isSupported() const { return m_type != GmcpModuleTypeEnum::UNKNOWN; }
    NODISCARD bool hasVersion() const
    {
        return m_nameVersion.version > DEFAULT_GMCP_MODULE_VERSION;
    }

public:
    NODISCARD GmcpModuleTypeEnum getType() const { return m_type; }
    NODISCARD GmcpModuleVersion getVersion() const { return m_nameVersion.version; }
    NODISCARD const std::string &getNormalizedName() const { return m_nameVersion.normalizedName; }

public:
    // TODO: tag this as latin1 or utf8
    NODISCARD std::string toStdString() const;
};

struct NODISCARD GmcpModuleHashFunction final
{
    std::size_t operator()(const GmcpModule &gmcp) const
    {
        return std::hash<std::string>{}(gmcp.getNormalizedName());
    }
};

struct NODISCARD GmcpModuleEqualTo final
{
    bool operator()(const GmcpModule &lhs, const GmcpModule &rhs) const
    {
        return lhs.getNormalizedName() == rhs.getNormalizedName();
    }
};

using GmcpModuleSet = std::unordered_set<GmcpModule, GmcpModuleHashFunction, GmcpModuleEqualTo>;
