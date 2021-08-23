#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstdint>
#include <optional>
#include <string>
#include <QByteArray>
#include <QString>

#include "../global/Flags.h"
#include "../global/TaggedString.h"

class ParseEvent;

// X(UPPER_CASE, CamelCase, "normalized name", "friendly name")
#define X_FOREACH_GMCP_MESSAGE_TYPE(X) \
    X(CORE_GOODBYE, CoreGoodbye, "core.goodbye", "Core.Goodbye") \
    X(CORE_HELLO, CoreHello, "core.hello", "Core.Hello") \
    X(CORE_SUPPORTS_ADD, CoreSupportsAdd, "core.supports.add", "Core.Supports.Add") \
    X(CORE_SUPPORTS_REMOVE, CoreSupportsRemove, "core.supports.remove", "Core.Supports.Remove") \
    X(CORE_SUPPORTS_SET, CoreSupportsSet, "core.supports.set", "Core.Supports.Set") \
    X(MMAPPER_GROUPTELL, MmapperGroupTell, "mmapper.grouptell", "MMapper.GroupTell") \
    /* define gmcp message types above */

enum class NODISCARD GmcpMessageTypeEnum {
    UNKNOWN = -1,
#define X_DECL_GMCP_MESSAGE_TYPE(UPPER_CASE, CamelCase, normalized, friendly) UPPER_CASE,
    X_FOREACH_GMCP_MESSAGE_TYPE(X_DECL_GMCP_MESSAGE_TYPE)
#undef X_DECL_GMCP_MESSAGE_TYPE
};

static constexpr const size_t NUM_GMCP_MESSAGES = 6u;
static_assert(NUM_GMCP_MESSAGES == static_cast<int>(GmcpMessageTypeEnum::MMAPPER_GROUPTELL) + 1);
DEFINE_ENUM_COUNT(GmcpMessageTypeEnum, NUM_GMCP_MESSAGES)

namespace tags {
struct NODISCARD GmcpMessageNameTag final
{};
} // namespace tags

using GmcpMessageName = TaggedString<tags::GmcpMessageNameTag>;

namespace tags {
struct NODISCARD GmcpJsonTag final
{};
} // namespace tags

using GmcpJson = TaggedStringUtf8<tags::GmcpJsonTag>;

class GmcpMessage final
{
private:
    GmcpMessageName name;
    std::optional<GmcpJson> json;
    GmcpMessageTypeEnum type = GmcpMessageTypeEnum::UNKNOWN;

public:
    explicit GmcpMessage(const std::string &package);
    explicit GmcpMessage(const std::string &package, const std::string &json);
    explicit GmcpMessage(const GmcpMessageTypeEnum type);
    explicit GmcpMessage(const GmcpMessageTypeEnum type, const std::string &json);
    explicit GmcpMessage(const GmcpMessageTypeEnum type, const QString &json);

public:
#define DECL_GETTERS_AND_SETTERS(UPPER_CASE, CamelCase, normalized, friendly) \
    inline bool is##CamelCase() const { return type == GmcpMessageTypeEnum::UPPER_CASE; }
    X_FOREACH_GMCP_MESSAGE_TYPE(DECL_GETTERS_AND_SETTERS)
#undef DECL_GETTERS_AND_SETTERS

public:
    NODISCARD GmcpMessageName getName() const { return name; }
    NODISCARD std::optional<GmcpJson> getJson() const { return json; }

public:
    NODISCARD QByteArray toRawBytes() const;
    NODISCARD static GmcpMessage fromRawBytes(const QByteArray &ba);
};
