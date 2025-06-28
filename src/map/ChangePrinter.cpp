// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ChangePrinter.h"

#include "../global/AnsiOstream.h"
#include "../global/PrintUtils.h"
#include "Map.h"

#include <ostream>

using namespace world_change_types;
using namespace exit_change_types;
using namespace room_change_types;
using namespace infomark_change_types;

namespace { // anonymous

constexpr size_t max_roomids_printed = 20;

constexpr RawAnsi const_color = getRawAnsi(AnsiColor16Enum::yellow);
constexpr RawAnsi error_color = getRawAnsi(AnsiColor16Enum::RED);
constexpr RawAnsi member_name_color = getRawAnsi(AnsiColor16Enum::cyan);
constexpr RawAnsi type_name_color = getRawAnsi(AnsiColor16Enum::BLUE);
constexpr RawAnsi warning_color = getRawAnsi(AnsiColor16Enum::YELLOW);

void print_string_color_quoted(AnsiOstream &aos, std::string_view sv)
{
    constexpr RawAnsi normalAnsi = getRawAnsi(AnsiColor16Enum::green);
    constexpr RawAnsi escapeAnsi = getRawAnsi(AnsiColor16Enum::yellow);
    aos.writeQuotedWithColor(normalAnsi, escapeAnsi, sv);
}

template<typename Type_, typename Tag_, typename Int_>
void writeTaggedInt(AnsiOstream &os,
                    const std::string_view typeName,
                    const TaggedInt<Tag_, Type_, Int_> tagged)
{
    os.writeWithColor(type_name_color, typeName);
    os << "{";
    os.writeWithColor(const_color, tagged.value());
    os << "}";
}

} // namespace

#define BEGIN_STRUCT_HELPER(name) if (StructHelper helper{*this, name})
#define HELPER_ADD_MEMBER(name) helper.add_member(#name, (change.name))
struct NODISCARD ChangePrinter::StructHelper final
{
private:
    ChangePrinter &m_cp;
    size_t m_elements = 0;

public:
    explicit StructHelper(ChangePrinter &cp, const std::string_view name)
        : m_cp(cp)
    {
        auto &os = m_cp.m_os;
        os.writeWithColor(type_name_color, name);
        os << "{";
    }
    ~StructHelper() { m_cp.m_os << "}"; }

private:
    void write_comma()
    {
        if (m_elements++ != 0) {
            m_cp.m_os << ", ";
        }
    }
    void write_member_name(const std::string_view name)
    {
        write_comma();
        auto &os = m_cp.m_os;
        os.writeWithColor(member_name_color, name);
    }
    void write_equals() { m_cp.m_os << " = "; }

public:
    template<typename T>
    void add_member(const std::string_view name, T &&value)
    {
        write_member_name(name);
        write_equals();
        m_cp.print(std::forward<T>(value));
    }
    template<typename K, typename V>
    void add_key_value(K &&key, V &&value)
    {
        write_comma();
        m_cp.print(std::forward<K>(key));
        write_equals();
        m_cp.print(std::forward<V>(value));
    }

    NODISCARD explicit operator bool() const { return true; }
};

#define BEGIN_FLAGS_HELPER(name) if (FlagsHelper helper{*this, name})
#define HELPER_ADD_FLAG(value) helper.add_flag(value)
struct NODISCARD ChangePrinter::FlagsHelper final
{
private:
    ChangePrinter &m_cp;
    size_t m_elements = 0;

public:
    FlagsHelper(ChangePrinter &cp, const std::string_view name)
        : m_cp(cp)
    {
        auto &os = m_cp.m_os;
        os.writeWithColor(type_name_color, name);
        os << "{";
    }
    ~FlagsHelper() { m_cp.m_os << "}"; }

private:
    void write_pipe()
    {
        if (m_elements++ != 0) {
            m_cp.m_os << " | ";
        }
    }

public:
    template<typename T>
    void add_flag(T &&thing)
    {
        write_pipe();
        m_cp.print(std::forward<T>(thing));
    }

    NODISCARD explicit operator bool() const { return true; }
};

ChangePrinter::ChangePrinter(Remap remap, AnsiOstream &os)
    : m_remap{std::move(remap)}
    , m_os(os)
{}

ChangePrinter::~ChangePrinter() = default;

void ChangePrinter::error()
{
    m_os.writeWithColor(error_color, "__ERROR__");
}

void ChangePrinter::print(const bool val)
{
    m_os.writeWithColor(const_color, val ? "true" : "false");
}

void ChangePrinter::print(const Coordinate &coord)
{
    auto &os = m_os;
    os.writeWithColor(type_name_color, "Coordinate");
    os << "{";
    os.writeWithColor(const_color, coord.x);
    os << ", ";
    os.writeWithColor(const_color, coord.y);
    os << ", ";
    os.writeWithColor(const_color, coord.z);
    os << "}";
}

void ChangePrinter::print(const ExitDirEnum dir)
{
    m_os.writeWithColor(const_color, to_string_view(dir));
}

void ChangePrinter::print(const ServerRoomId serverId)
{
    if (serverId == INVALID_SERVER_ROOMID) {
        m_os.writeWithColor(error_color, "INVALID_SERVER_ID");
    } else {
        writeTaggedInt(m_os, "ServerRoomId", serverId);
    }
}

void ChangePrinter::print(const RoomId room)
{
    if (room == INVALID_ROOMID) {
        m_os.writeWithColor(error_color, "INVALID_ROOMID");
    } else if (const auto ext = m_remap(room); ext != INVALID_EXTERNAL_ROOMID) {
        print(ext);
    } else {
        m_os.writeWithColor(error_color, "UnknownRoomId");
        m_os << "{";
        m_os.writeWithColor(const_color, room.asUint32());
        m_os << "}";
    }
}

void ChangePrinter::print(const ExternalRoomId ext)
{
    if (ext == INVALID_EXTERNAL_ROOMID) {
        m_os.writeWithColor(error_color, "INVALID_EXTERNAL_ROOMID");
    } else {
        writeTaggedInt(m_os, "ExternalRoomId", ext);
    }
}

void ChangePrinter::print(const DoorName &name)
{
    print_string_color_quoted(m_os, name.getStdStringViewUtf8());
}

void ChangePrinter::print(const RoomContents &name)
{
    print_string_color_quoted(m_os, name.getStdStringViewUtf8());
}

void ChangePrinter::print(const RoomArea &name)
{
    print_string_color_quoted(m_os, name.getStdStringViewUtf8());
}

void ChangePrinter::print(const RoomName &name)
{
    print_string_color_quoted(m_os, name.getStdStringViewUtf8());
}

void ChangePrinter::print(const RoomNote &name)
{
    print_string_color_quoted(m_os, name.getStdStringViewUtf8());
}

void ChangePrinter::print(const RoomDesc &name)
{
    print_string_color_quoted(m_os, name.getStdStringViewUtf8());
}

void ChangePrinter::print(const ChangeTypeEnum type)
{
#define X_CASE(x) \
    case ChangeTypeEnum::x: \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_ChangeTypeEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const DirectSunlightEnum type)
{
    // use "saner" strings
    switch (type) {
    case DirectSunlightEnum::UNKNOWN:
        m_os.writeWithColor(const_color, "UNKNOWN");
        return;
    case DirectSunlightEnum::SAW_DIRECT_SUN:
        m_os.writeWithColor(const_color, "SUN");
        return;
    case DirectSunlightEnum::SAW_NO_DIRECT_SUN:
        m_os.writeWithColor(const_color, "DARK");
        return;
    }
    error();
}

void ChangePrinter::print(const DoorFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    case DoorFlagEnum::UPPER_CASE: \
        m_os.writeWithColor(const_color, #UPPER_CASE); \
        return;

    switch (flag) {
        XFOREACH_DOOR_FLAG(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const ExitFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    case ExitFlagEnum::UPPER_CASE: \
        m_os.writeWithColor(const_color, #UPPER_CASE); \
        return;

    switch (flag) {
        XFOREACH_EXIT_FLAG(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const FlagChangeEnum type)
{
#define X_CASE(X) \
    case FlagChangeEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (type) {
        XFOREACH_FlagChangeEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const FlagModifyModeEnum mode)
{
#define X_CASE(X) \
    case FlagModifyModeEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (mode) {
        XFOREACH_FlagModifyModeEnum(X_CASE)
    }

    error();
#undef X_CASE
}

void ChangePrinter::print(const PositionChangeEnum type)
{
#define X_CASE(X) \
    case PositionChangeEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (type) {
        XFOREACH_PositionChangeEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const PromptFogEnum type)
{
#define X_CASE(X) \
    case PromptFogEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (type) {
        XFOREACH_PROMPT_FOG_ENUM(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const PromptWeatherEnum type)
{
#define X_CASE(X) \
    case PromptWeatherEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (type) {
        XFOREACH_PROMPT_WEATHER_ENUM(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const UpdateTypeEnum type)
{
#define X_CASE(x) \
    case UpdateTypeEnum::x: \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_UpdateTypeEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomAlignEnum type)
{
#define X_CASE(x) \
    case (RoomAlignEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_RoomAlignEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomLoadFlagEnum flag)
{
#define X_CASE(X) \
    case RoomLoadFlagEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (flag) {
        XFOREACH_ROOM_LOAD_FLAG(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomMobFlagEnum flag)
{
#define X_CASE(X) \
    case RoomMobFlagEnum::X: \
        m_os.writeWithColor(const_color, #X); \
        return;

    switch (flag) {
        XFOREACH_ROOM_MOB_FLAG(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomLightEnum type)
{
#define X_CASE(x) \
    case (RoomLightEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_RoomLightEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomPortableEnum type)
{
#define X_CASE(x) \
    case (RoomPortableEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_RoomPortableEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomRidableEnum type)
{
#define X_CASE(x) \
    case (RoomRidableEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_RoomRidableEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomSundeathEnum type)
{
#define X_CASE(x) \
    case (RoomSundeathEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_RoomSundeathEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const RoomTerrainEnum type)
{
#define X_CASE(x) \
    case (RoomTerrainEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (type) {
        XFOREACH_RoomTerrainEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const WaysEnum ways)
{
#define X_CASE(x) \
    case (WaysEnum::x): \
        m_os.writeWithColor(const_color, #x); \
        return;

    switch (ways) {
        XFOREACH_WaysEnum(X_CASE)
    }
    error();
#undef X_CASE
}

void ChangePrinter::print(const ConnectedRoomFlagsType flags)
{
    BEGIN_STRUCT_HELPER("ConnectedRoomFlags")
    {
        if (!flags.isValid()) {
            helper.add_member("INVALID", true);
        } else {
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                helper.add_key_value(dir, flags.getDirectSunlight(dir));
            }
        }
    }
}

void ChangePrinter::print(const ExitsFlagsType flags)
{
    BEGIN_STRUCT_HELPER("ExitsFlags")
    {
        if (!flags.isValid()) {
            helper.add_member("INVALID", true);
        } else {
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                helper.add_key_value(dir, flags.get(dir));
            }
        }
    }
}

void ChangePrinter::print(const DoorFlags doorFlags)
{
    BEGIN_FLAGS_HELPER("DoorFlags")
    {
        for (const DoorFlagEnum flag : doorFlags) {
            HELPER_ADD_FLAG(flag);
        }
    }
}

void ChangePrinter::print(const ExitFlags exitFlags)
{
    BEGIN_FLAGS_HELPER("ExitFlags")
    {
        for (const ExitFlagEnum flag : exitFlags) {
            HELPER_ADD_FLAG(flag);
        }
    }
}

void ChangePrinter::print(const PromptFlagsType flags)
{
    BEGIN_STRUCT_HELPER("PromptFlagsType")
    {
        if (!flags.isValid()) {
            helper.add_member("INVALID", true);
        } else {
            helper.add_member("fog_type", flags.getFogType());
            helper.add_member("weather_type", flags.getWeatherType());
            helper.add_member("lit", flags.isLit());
            helper.add_member("dark", flags.isDark());
        }
    }
}

void ChangePrinter::print(const RoomLoadFlags loadFlags)
{
    BEGIN_FLAGS_HELPER("RoomLoadFlags")
    {
        for (const RoomLoadFlagEnum flag : loadFlags) {
            HELPER_ADD_FLAG(flag);
        }
    }
}

void ChangePrinter::print(const RoomMobFlags mobFlags)
{
    BEGIN_FLAGS_HELPER("RoomMobFlags")
    {
        for (const RoomMobFlagEnum flag : mobFlags) {
            HELPER_ADD_FLAG(flag);
        }
    }
}

void ChangePrinter::print(const ExitFieldVariant &var)
{
    switch (var.getType()) {
    case ExitFieldEnum::DOOR_NAME: {
        auto &os = m_os;
        os.writeWithColor(type_name_color, "DoorName");
        os << "{";
        print(var.getDoorName());
        os << "}";
        return;
    }
    case ExitFieldEnum::EXIT_FLAGS:
        print(var.getExitFlags());
        return;
    case ExitFieldEnum::DOOR_FLAGS:
        print(var.getDoorFlags());
        return;
    }
    error();
}

void ChangePrinter::print(const RoomFieldVariant &var)
{
#define X_NOP()
#define X_PRINT(UPPER_CASE, CamelCase, Type) \
    case RoomFieldEnum::UPPER_CASE: { \
        auto &os = m_os; \
        os.writeWithColor(type_name_color, #UPPER_CASE); \
        os << "{"; \
        print(var.get##CamelCase()); \
        os << "}"; \
        return; \
    }
    switch (var.getType()) {
        XFOREACH_ROOM_FIELD(X_PRINT, X_NOP)
    case RoomFieldEnum::RESERVED:
        m_os.writeWithColor(error_color, "RESERVED");
        return;
    }
    error();
#undef X_PRINT
#undef X_NOP
}

void ChangePrinter::print(const InfomarkId id)
{
    if (id == INVALID_INFOMARK_ID) {
        m_os.writeWithColor(error_color, "INVALID_INFOMARK_ID");
    } else {
        writeTaggedInt(m_os, "InfomarkId", id);
    }
}

void ChangePrinter::print(const InfomarkClassEnum type)
{
#define CASE(UPPER, s) \
    do { \
    case InfomarkClassEnum::UPPER: \
        return m_os.writeWithColor(const_color, s); \
    } while (false)
    switch (type) {
        // TODO: Move this into infomark.h XFOREACH
        CASE(GENERIC, "generic");
        CASE(HERB, "herb");
        CASE(RIVER, "river");
        CASE(PLACE, "place");
        CASE(MOB, "mob");
        CASE(COMMENT, "comment");
        CASE(ROAD, "road");
        CASE(OBJECT, "object");
        CASE(ACTION, "action");
        CASE(LOCALITY, "locality");
    }
#undef CASE
}

void ChangePrinter::print(const InfomarkTypeEnum type)
{
#define CASE(UPPER, s) \
    do { \
    case InfomarkTypeEnum::UPPER: \
        return m_os.writeWithColor(const_color, s); \
    } while (false)
    switch (type) {
        // TODO: Move this into infomark.h XFOREACH
        CASE(TEXT, "text");
        CASE(LINE, "line");
        CASE(ARROW, "arrow");
    }
#undef CASE
}

void ChangePrinter::print(const InfomarkText &text)
{
    print_string_color_quoted(m_os, text.getStdStringViewUtf8());
}

void ChangePrinter::print(const RawInfomark &mark)
{
    BEGIN_STRUCT_HELPER("RawInfomark")
    {
#define X_CASE(Type, Prop, Init) helper.add_member(#Prop, mark.get##Prop());
        XFOREACH_INFOMARK_PROPERTY(X_CASE)
#undef X_CASE
    }
}

void ChangePrinter::print(const RoomIdSet &set)
{
    auto &os = m_os;
    auto prefix = "";
    os.writeWithColor(type_name_color, "RoomIdSet");
    os << "{";
    size_t rooms_printed = 0;
    for (auto id : set) {
        os << prefix;
        prefix = ", ";
        if (rooms_printed++ >= max_roomids_printed) {
            os.writeWithColor(warning_color, "...");
            break;
        }
        print(id);
    }
    os << "}";
}

void ChangePrinter::print(const ParseEvent &event)
{
    BEGIN_STRUCT_HELPER("ParseEvent")
    {
        helper.add_member("name", event.getRoomName());
        helper.add_member("desc", event.getRoomDesc());
        helper.add_member("contents", event.getRoomContents());
        helper.add_member("exits_flags", event.getExitsFlags());
        helper.add_member("prompt_flags", event.getPromptFlags());
        helper.add_member("connected_room_flags", event.getConnectedRoomFlags());
    }
}

void ChangePrinter::virt_accept(const CompactRoomIds &)
{
    m_os << "CompactRoomIds{}";
}

void ChangePrinter::virt_accept(const RemoveAllDoorNames &)
{
    m_os << "RemoveAllDoorNames{}";
}

void ChangePrinter::virt_accept(const GenerateBaseMap &)
{
    m_os << "GenerateBaseMap{}";
}

void ChangePrinter::virt_accept(const AddPermanentRoom &change)
{
    BEGIN_STRUCT_HELPER("AddPermanentRoom")
    {
        //
        HELPER_ADD_MEMBER(position);
    }
}

void ChangePrinter::virt_accept(const AddRoom2 &change)
{
    BEGIN_STRUCT_HELPER("AddRoom2")
    {
        // HELPER_ADD_MEMBER(server_id);
        HELPER_ADD_MEMBER(position);
        HELPER_ADD_MEMBER(event);
    }
}

void ChangePrinter::virt_accept(const UndeleteRoom &change)
{
    BEGIN_STRUCT_HELPER("UndeleteRoom")
    {
        HELPER_ADD_MEMBER(room);
        // REVISIT: this change information isn't shown.
        // HELPER_ADD_MEMBER(raw);
    }
}

void ChangePrinter::virt_accept(const SetServerId &change)
{
    BEGIN_STRUCT_HELPER("SetServerId")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(server_id);
    }
}

void ChangePrinter::virt_accept(const MakePermanent &change)
{
    BEGIN_STRUCT_HELPER("MakePermanent")
    {
        HELPER_ADD_MEMBER(room);
    }
}

void ChangePrinter::virt_accept(const MergeRelative &change)
{
    BEGIN_STRUCT_HELPER("MergeRelative")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(offset);
    }
}

void ChangePrinter::virt_accept(const ModifyRoomFlags &change)
{
    BEGIN_STRUCT_HELPER("ModifyRoomFlags")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(field);
        HELPER_ADD_MEMBER(mode);
    }
}

void ChangePrinter::virt_accept(const MoveRelative &change)
{
    BEGIN_STRUCT_HELPER("MoveRelative")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(offset);
    }
}

void ChangePrinter::virt_accept(const MoveRelative2 &change)
{
    BEGIN_STRUCT_HELPER("MoveRelative2")
    {
        HELPER_ADD_MEMBER(rooms);
        HELPER_ADD_MEMBER(offset);
    }
}

void ChangePrinter::virt_accept(const TryMoveCloseTo &change)
{
    BEGIN_STRUCT_HELPER("TryMoveCloseTo")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(desiredPosition);
    }
}

void ChangePrinter::virt_accept(const RemoveRoom &change)
{
    BEGIN_STRUCT_HELPER("RemoveRoom")
    {
        HELPER_ADD_MEMBER(room);
    }
}

void ChangePrinter::virt_accept(const Update &change)
{
    BEGIN_STRUCT_HELPER("Update")
    {
        HELPER_ADD_MEMBER(room);
        helper.add_member("change", change.event);
        HELPER_ADD_MEMBER(type);
    }
}

void ChangePrinter::virt_accept(const ModifyExitConnection &change)
{
    BEGIN_STRUCT_HELPER("ModifyExitConnection")
    {
        HELPER_ADD_MEMBER(type);
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(dir);
        HELPER_ADD_MEMBER(to);
        HELPER_ADD_MEMBER(ways);
    }
}

void ChangePrinter::virt_accept(const ModifyExitFlags &change)
{
    BEGIN_STRUCT_HELPER("ModifyExitFlags")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(dir);
        HELPER_ADD_MEMBER(field);
        HELPER_ADD_MEMBER(mode);
    }
}

void ChangePrinter::virt_accept(const NukeExit &change)
{
    BEGIN_STRUCT_HELPER("NukeExit")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(dir);
        HELPER_ADD_MEMBER(ways);
    }
}

void ChangePrinter::virt_accept(const SetDoorFlags &change)
{
    BEGIN_STRUCT_HELPER("SetExitFlags")
    {
        HELPER_ADD_MEMBER(type);
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(dir);
        HELPER_ADD_MEMBER(flags);
    }
}

void ChangePrinter::virt_accept(const SetDoorName &change)
{
    BEGIN_STRUCT_HELPER("SetDoorName")
    {
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(dir);
        HELPER_ADD_MEMBER(name);
    }
}

void ChangePrinter::virt_accept(const SetExitFlags &change)
{
    BEGIN_STRUCT_HELPER("SetExitFlags")
    {
        HELPER_ADD_MEMBER(type);
        HELPER_ADD_MEMBER(room);
        HELPER_ADD_MEMBER(dir);
        HELPER_ADD_MEMBER(flags);
    }
}

void ChangePrinter::virt_accept(const AddInfomark &change)
{
    BEGIN_STRUCT_HELPER("AddInfomark")
    {
        HELPER_ADD_MEMBER(fields);
    }
}

void ChangePrinter::virt_accept(const RemoveInfomark &change)
{
    BEGIN_STRUCT_HELPER("RemoveInfomark")
    {
        HELPER_ADD_MEMBER(id);
    }
}

void ChangePrinter::virt_accept(const UpdateInfomark &change)
{
    BEGIN_STRUCT_HELPER("UpdateInfomark")
    {
        HELPER_ADD_MEMBER(id);
        HELPER_ADD_MEMBER(fields);
    }
}
