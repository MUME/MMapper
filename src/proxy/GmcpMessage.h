#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Flags.h"
#include "../global/TaggedString.h"

#include <cstdint>
#include <optional>
#include <string>

#include <QByteArray>
#include <QJsonDocument>
#include <QString>

class ParseEvent;

// X(UPPER_CASE, CamelCase, "normalized name", "friendly name")
#define X_FOREACH_GMCP_MESSAGE_TYPE(X) \
    X(CHAR_NAME, CharName, "char.name", "Char.Name") \
    X(CHAR_STATUSVARS, CharStatusVars, "char.statusvars", "Char.StatusVars") \
    X(CHAR_VITALS, CharVitals, "char.vitals", "Char.Vitals") \
    X(CORE_GOODBYE, CoreGoodbye, "core.goodbye", "Core.Goodbye") \
    X(CORE_HELLO, CoreHello, "core.hello", "Core.Hello") \
    X(CORE_SUPPORTS_ADD, CoreSupportsAdd, "core.supports.add", "Core.Supports.Add") \
    X(CORE_SUPPORTS_REMOVE, CoreSupportsRemove, "core.supports.remove", "Core.Supports.Remove") \
    X(CORE_SUPPORTS_SET, CoreSupportsSet, "core.supports.set", "Core.Supports.Set") \
    X(EVENT_DARKNESS, EventDarkness, "event.darkness", "Event.Darkness") \
    X(EVENT_MOON, EventMoon, "event.moon", "Event.Moon") \
    X(EVENT_SUN, EventSun, "event.sun", "Event.Sun") \
    X(EXTERNAL_DISCORD_HELLO, \
      ExternalDiscordHello, \
      "external.discord.hello", \
      "External.Discord.Hello") \
    X(MMAPPER_COMM_GROUPTELL, \
      MmapperCommGroupTell, \
      "mmapper.comm.grouptell", \
      "MMapper.Comm.GroupTell") \
    X(ROOM_CHARS_ADD, RoomCharsAdd, "room.chars.add", "Room.Chars.Add") \
    X(ROOM_CHARS_REMOVE, RoomCharsRemove, "room.chars.remove", "Room.Chars.Remove") \
    X(ROOM_CHARS_SET, RoomCharsSet, "room.chars.set", "Room.Chars.Set") \
    X(ROOM_CHARS_UPDATE, RoomCharsUpdate, "room.chars.update", "Room.Chars.Update") \
    /* define gmcp message types above */

enum class NODISCARD GmcpMessageTypeEnum {
    UNKNOWN = -1,
#define X_DECL_GMCP_MESSAGE_TYPE(UPPER_CASE, CamelCase, normalized, friendly) UPPER_CASE,
    X_FOREACH_GMCP_MESSAGE_TYPE(X_DECL_GMCP_MESSAGE_TYPE)
#undef X_DECL_GMCP_MESSAGE_TYPE
};

static constexpr const size_t NUM_GMCP_MESSAGES = 17u;
static_assert(NUM_GMCP_MESSAGES == static_cast<int>(GmcpMessageTypeEnum::ROOM_CHARS_UPDATE) + 1);
DEFINE_ENUM_COUNT(GmcpMessageTypeEnum, NUM_GMCP_MESSAGES)

namespace tags {
struct NODISCARD GmcpMessageNameTag final
{};
} // namespace tags

using GmcpMessageName = TaggedStringLatin1<tags::GmcpMessageNameTag>;

namespace tags {
struct NODISCARD GmcpJsonTag final
{};
} // namespace tags

using GmcpJson = TaggedStringUtf8<tags::GmcpJsonTag>;
using GmcpJsonDocument = QJsonDocument;

class NODISCARD GmcpMessage final
{
private:
    GmcpMessageName m_name;
    std::optional<GmcpJson> m_json;
    std::optional<GmcpJsonDocument> m_document;
    GmcpMessageTypeEnum m_type = GmcpMessageTypeEnum::UNKNOWN;

public:
    explicit GmcpMessage() = default;
    explicit GmcpMessage(GmcpMessageName package);
    explicit GmcpMessage(GmcpMessageName package, GmcpJson json);
    explicit GmcpMessage(GmcpMessageTypeEnum type);
    explicit GmcpMessage(GmcpMessageTypeEnum type, GmcpJson json);

public:
#define DECL_GETTERS_AND_SETTERS(UPPER_CASE, CamelCase, normalized, friendly) \
    NODISCARD inline bool is##CamelCase() const \
    { \
        return m_type == GmcpMessageTypeEnum::UPPER_CASE; \
    }
    X_FOREACH_GMCP_MESSAGE_TYPE(DECL_GETTERS_AND_SETTERS)
#undef DECL_GETTERS_AND_SETTERS

public:
    NODISCARD const GmcpMessageName &getName() const
    {
        return m_name;
    }
    NODISCARD const std::optional<GmcpJson> &getJson() const
    {
        return m_json;
    }
    NODISCARD const std::optional<GmcpJsonDocument> &getJsonDocument() const
    {
        return m_document;
    }

public:
    NODISCARD QByteArray toRawBytes() const;
    NODISCARD static GmcpMessage fromRawBytes(const QByteArray &ba);
};
