#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "exit.h"
#include "room.h"

#include <sstream>

template<typename CRTP>
struct NODISCARD ExitFieldsGetters
{
protected:
    ExitFieldsGetters() = default;

private:
    NODISCARD const auto &crtp_get_fields() const
    {
        return static_cast<const CRTP &>(*this).getExitFields();
    }
    NODISCARD decltype(auto) crtp_get_in() const
    {
        return static_cast<const CRTP &>(*this).getIncomingSet();
    }
    NODISCARD decltype(auto) crtp_get_out() const
    {
        return static_cast<const CRTP &>(*this).getOutgoingSet();
    }

public:
#define X_IMPL_GETTER(_Type, _Prop, _OptInit) \
    NODISCARD const _Type &get##_Type() const \
    { \
        return crtp_get_fields()._Prop; \
    }
    XFOREACH_EXIT_PROPERTY(X_IMPL_GETTER)
#undef X_IMPL_GETTER

#define X_IMPL_GETTER(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool exitIs##CamelCase() const \
    { \
        return crtp_get_fields().exitFlags.is##CamelCase(); \
    }
    XFOREACH_EXIT_FLAG(X_IMPL_GETTER)
#undef X_IMPL_GETTER

#define X_IMPL_GETTER(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool doorIs##CamelCase() const \
    { \
        return exitIsDoor() && crtp_get_fields().doorFlags.is##CamelCase(); \
    }
    XFOREACH_DOOR_FLAG(X_IMPL_GETTER)
#undef X_IMPL_GETTER

public:
    NODISCARD inline bool hasDoorName() const
    {
        return exitIsDoor() && !getDoorName().isEmpty();
    }

public:
    /* older aliases */
    DEPRECATED_MSG("use exitIsDoor()")
    NODISCARD inline bool isDoor() const = delete;
    DEPRECATED_MSG("use exitIsExit()")
    NODISCARD inline bool isExit() const = delete;
    DEPRECATED_MSG("use doorIsHidden()")
    NODISCARD inline bool isHiddenExit() const = delete;

public:
#define X_DECL_INOUT_FNS(lower, SnakeCase) \
    NODISCARD bool lower##IsUnique() const \
    { \
        return crtp_get_##lower().size() == 1; \
    } \
    NODISCARD bool contains##SnakeCase(RoomId id) const \
    { \
        return crtp_get_##lower().contains(id); \
    } \
    NODISCARD bool lower##IsEmpty() const \
    { \
        return crtp_get_##lower().empty(); \
    } \
    NODISCARD auto lower##First() const \
    { \
        return crtp_get_##lower().first(); \
    }

    X_DECL_INOUT_FNS(out, Out)
    X_DECL_INOUT_FNS(in, In)

#undef X_DECL_INOUT_FNS
};

template<typename CRTP>
struct NODISCARD ExitFieldsSetters
{
protected:
    ExitFieldsSetters() = default;

private:
    NODISCARD auto &crtp_get_fields() { return static_cast<CRTP &>(*this).getExitFields(); }

public:
#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void set##_Type(_Type value) \
    { \
        crtp_get_fields()._Prop = std::move(value); \
    }
    XFOREACH_EXIT_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS

#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void add##_Type(_Type value) \
    { \
        crtp_get_fields()._Prop |= value; \
    } \
    void remove##_Type(_Type value) \
    { \
        crtp_get_fields()._Prop &= ~value; \
    }
    XFOREACH_EXIT_FLAG_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS

#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void add##_Type(_Type::Flag value) \
    { \
        using Type = _Type; \
        add##_Type(Type{value}); \
    } \
    void remove##_Type(_Type::Flag value) \
    { \
        using Type = _Type; \
        remove##_Type(Type{value}); \
    }
    XFOREACH_EXIT_FLAG_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS

    void clearFields()
    {
        static_assert(std::is_reference_v<decltype(crtp_get_fields())>);
        crtp_get_fields() = {};
    }
};

// FIXME: Names for getters and setters are too visually similar!
template<typename CRTP>
struct NODISCARD RoomFieldsGetters
{
protected:
    RoomFieldsGetters() = default;

private:
    NODISCARD const auto &crtp_get_fields() const
    {
        return static_cast<const CRTP &>(*this).getRoomFields();
    }

public:
#define X_IMPL_GETTER(_Type, _Prop, _OptInit) \
    NODISCARD const _Type &get##_Prop() const \
    { \
        return crtp_get_fields()._Prop; \
    }
    XFOREACH_ROOM_PROPERTY(X_IMPL_GETTER)
#undef X_IMPL_GETTER
};

template<typename CRTP>
struct NODISCARD RoomFieldsSetters
{
protected:
    RoomFieldsSetters() = default;

private:
    NODISCARD auto &crtp_get_fields() { return static_cast<CRTP &>(*this).getRoomFields(); }

public:
#define X_IMPL_SETTER(_Type, _Prop, _OptInit) \
    void set##_Prop(_Type value) \
    { \
        crtp_get_fields()._Prop = std::move(value); \
    }
    XFOREACH_ROOM_PROPERTY(X_IMPL_SETTER)
#undef X_IMPL_SETTER

#define X_IMPL_SETTER(_Type, _Prop, _OptInit) \
    void add##_Prop(const _Type value) \
    { \
        crtp_get_fields()._Prop |= value; \
    } \
    void remove##_Prop(const _Type value) \
    { \
        crtp_get_fields()._Prop &= ~value; \
    }
    XFOREACH_ROOM_FLAG_PROPERTY(X_IMPL_SETTER)
#undef X_IMPL_SETTER

#define X_IMPL_SETTER(_Type, _Prop, _OptInit) \
    void add##_Prop(const _Type::Flag value) \
    { \
        using Type = _Type; \
        add##_Prop(Type{value}); \
    } \
    void remove##_Prop(const _Type::Flag value) \
    { \
        using Type = _Type; \
        remove##_Prop(Type{value}); \
    }
    XFOREACH_ROOM_FLAG_PROPERTY(X_IMPL_SETTER)
#undef X_IMPL_SETTER
};

template<typename CRTP>
struct NODISCARD RoomExitFieldsGetters
{
protected:
    RoomExitFieldsGetters() = default;

private:
    // Expect a const reference or a copy (necessary for fat-pointer proxy objects).
    NODISCARD decltype(auto) crtp_get_exit(const ExitDirEnum dir) const
    {
        return static_cast<const CRTP &>(*this).getExit(dir);
    }

public:
#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD const auto &get##_Type(const ExitDirEnum dir) const \
    { \
        return crtp_get_exit(dir).get##_Type(); \
    }
    XFOREACH_EXIT_FLAG_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS

    NODISCARD DoorName getDoorName(const ExitDirEnum dir) const
    {
        return crtp_get_exit(dir).getDoorName();
    }
};

template<typename CRTP>
struct NODISCARD RoomExitFieldsSetters
{
protected:
    RoomExitFieldsSetters() = default;

private:
    NODISCARD auto &crtp_get_exit(const ExitDirEnum dir)
    {
        return static_cast<CRTP &>(*this).getExit(dir);
    }

public:
#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void set##_Type(const ExitDirEnum dir, _Type value) \
    { \
        crtp_get_exit(dir).set##_Type(std::move(value)); \
    }
    XFOREACH_EXIT_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS
#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void add##_Type(const ExitDirEnum dir, const _Type value) \
    { \
        crtp_get_exit(dir).add##_Type(value); \
    } \
    void remove##_Type(const ExitDirEnum dir, const _Type value) \
    { \
        crtp_get_exit(dir).remove##_Type(value); \
    }
    XFOREACH_EXIT_FLAG_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS
#define X_DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void add##_Type(const ExitDirEnum dir, const _Type::Flag value) \
    { \
        crtp_get_exit(dir).add##_Type(value); \
    } \
    void remove##_Type(const ExitDirEnum dir, const _Type::Flag value) \
    { \
        crtp_get_exit(dir).remove##_Type(value); \
    }
    XFOREACH_EXIT_FLAG_PROPERTY(X_DEFINE_SETTERS)
#undef X_DEFINE_SETTERS
};

template<typename Room_>
std::string toStdStringUtf8(const Room_ &r)
{
    // Try to get better error messages if you use this on the wrong type.
    static_assert(std::is_base_of_v<RoomFieldsGetters<Room_>, Room_>);
    using Exit_ = std::decay_t<decltype(r.getExit(std::declval<ExitDirEnum>()))>;
    static_assert(std::is_base_of_v<ExitFieldsGetters<Exit_>, Exit_>);
    return toStdStringUtf8_unsafe(r);
}

template<typename Room_>
std::string toStdStringUtf8_unsafe(const Room_ &r)
{
    std::ostringstream ss;
    ss << r.getName().getStdStringViewUtf8() << "\n"
       << r.getDescription().getStdStringViewUtf8() << r.getContents().getStdStringViewUtf8();

    ss << "Exits:";
    for (const ExitDirEnum j : ALL_EXITS7) {
        const auto &ex = r.getExit(j);
        if (!ex.exitIsExit()) {
            continue;
        }

        ss << " ";

        const bool climb = ex.exitIsClimb();
        if (climb) {
            ss << "|";
        }
        const bool door = ex.exitIsDoor();
        if (door) {
            ss << "(";
        }
        ss << to_string_view(j);
        if (door) {
            const auto doorName = ex.getDoorName();
            if (!doorName.isEmpty()) {
                ss << "/" << doorName.getStdStringViewUtf8();
            }
            ss << ")";
        }
        if (climb) {
            ss << "|";
        }
    }
    ss << ".\n";
    if (!r.getNote().isEmpty()) {
        ss << "Note: " << r.getNote().getStdStringViewUtf8();
    }

    return std::move(ss).str();
}
