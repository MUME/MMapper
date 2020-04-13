#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"

// X(UPPER_CASE, CamelCase, "normalized name", "friendly name")
#define X_FOREACH_GMCP_MODULE_TYPE(X) /* define gmcp module types above */

enum class GmcpModuleTypeEnum {
    UNKNOWN = -1,
#define X_DECL_GMCP_MODULE_TYPE(UPPER_CASE, CamelCase, normalized, friendly) UPPER_CASE,
    X_FOREACH_GMCP_MODULE_TYPE(X_DECL_GMCP_MODULE_TYPE)
#undef X_DECL_GMCP_MODULE_TYPE
};

static constexpr const size_t NUM_GMCP_MODULES = 0u;
static_assert(NUM_GMCP_MODULES == static_cast<int>(GmcpModuleTypeEnum::UNKNOWN) + 1);
DEFINE_ENUM_COUNT(GmcpModuleTypeEnum, NUM_GMCP_MODULES)

struct GmcpModuleVersion final
{
private:
    uint32_t value = 0;

public:
    GmcpModuleVersion() = default;
    constexpr explicit GmcpModuleVersion(uint32_t value) noexcept
        : value{value}
    {}
    inline constexpr explicit operator uint32_t() const { return value; }
    inline constexpr uint32_t asUint32() const { return value; }
    inline constexpr bool operator<(GmcpModuleVersion rhs) const { return value < rhs.value; }
    inline constexpr bool operator>(GmcpModuleVersion rhs) const { return value > rhs.value; }
    inline constexpr bool operator<=(GmcpModuleVersion rhs) const { return value <= rhs.value; }
    inline constexpr bool operator>=(GmcpModuleVersion rhs) const { return value >= rhs.value; }
    inline constexpr bool operator==(GmcpModuleVersion rhs) const { return value == rhs.value; }
    inline constexpr bool operator!=(GmcpModuleVersion rhs) const { return value != rhs.value; }
};
static constexpr const GmcpModuleVersion DEFAULT_GMCP_MODULE_VERSION{0};

using GmcpModuleVersionList
    = EnumIndexedArray<GmcpModuleVersion, GmcpModuleTypeEnum, NUM_GMCP_MODULES>;

class QString;

class GmcpModule final
{
private:
    std::string normalizedName;
    GmcpModuleVersion version = DEFAULT_GMCP_MODULE_VERSION;
    GmcpModuleTypeEnum type = GmcpModuleTypeEnum::UNKNOWN;

public:
    explicit GmcpModule(const QString &);
    explicit GmcpModule(const std::string &);
    DEFAULT_RULE_OF_5(GmcpModule);

public:
    bool isSupported() const { return type != GmcpModuleTypeEnum::UNKNOWN; }
    bool hasVersion() const { return version > DEFAULT_GMCP_MODULE_VERSION; }

public:
    GmcpModuleTypeEnum getType() const { return type; }
    GmcpModuleVersion getVersion() const { return version; }
    std::string getNormalizedName() const { return normalizedName; }

public:
    std::string toStdString() const;
};

struct GmcpModuleHashFunction
{
    std::size_t operator()(const GmcpModule &gmcp) const
    {
        return std::hash<std::string>{}(gmcp.getNormalizedName());
    }
};

struct GmcpModuleEqualTo
{
    bool operator()(const GmcpModule &lhs, const GmcpModule &rhs) const
    {
        return lhs.getNormalizedName() == rhs.getNormalizedName();
    }
};

using GmcpModuleSet = std::unordered_set<GmcpModule, GmcpModuleHashFunction, GmcpModuleEqualTo>;
