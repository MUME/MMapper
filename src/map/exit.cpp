// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "exit.h"

#include "../global/Flags.h"
#include "DoorFlags.h"
#include "ExitFieldVariant.h"
#include "ExitFlags.h"

#define DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void Exit::set##_Type(_Type value) \
    { \
        m_fields._Prop = std::move(value); \
    }
XFOREACH_EXIT_PROPERTY(DEFINE_SETTERS)
#undef DEFINE_SETTERS

// bool Exit::exitXXX() const
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool Exit::exitIs##CamelCase() const \
    { \
        return getExitFlags().is##CamelCase(); \
    }
X_FOREACH_EXIT_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

// bool Exit::doorIsXXX() const
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool Exit::doorIs##CamelCase() const \
    { \
        return exitIsDoor() && getDoorFlags().is##CamelCase(); \
    }
X_FOREACH_DOOR_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

bool Exit::operator==(const Exit &rhs) const
{
    return m_fields.doorName == rhs.m_fields.doorName
           && m_fields.exitFlags == rhs.m_fields.exitFlags
           && m_fields.doorFlags == rhs.m_fields.doorFlags && incoming == rhs.incoming
           && outgoing == rhs.outgoing;
}

bool Exit::operator!=(const Exit &rhs) const
{
    return !(rhs == *this);
}

template<typename T>
inline void maybeUpdate(T &x, const T &value)
{
    if (x != value) {
        x = value;
    }
}

void Exit::assignFrom(const Exit &rhs)
{
    maybeUpdate(m_fields.doorName, rhs.m_fields.doorName);
    m_fields.doorFlags = rhs.m_fields.doorFlags; // no allocation required
    m_fields.exitFlags = rhs.m_fields.exitFlags; // no allocation required
    maybeUpdate(incoming, rhs.incoming);
    maybeUpdate(outgoing, rhs.outgoing);
    assert(*this == rhs);
}
