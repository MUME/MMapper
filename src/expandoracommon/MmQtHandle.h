#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_MMQTHANDLE_H
#define MMAPPER_MMQTHANDLE_H

#include <cstddef>
#include <memory>

#include "../global/NullPointerException.h"
#include "../global/RuleOf5.h"
#include "../global/utils.h"

/// Handle used for QT signals.
///
/// This is effectively just a thin wrapper around shared_ptr<T>
/// that helps guarantee the shared_ptr can never be nullptr.
///
/// NOTE: This class purposely avoids wrapping shared_ptr's operator->(),
/// operator*(), reset(), etc.
template<typename _T>
class MmQtHandle final
{
public:
    using contained_type = _T;
    using shared_type = std::shared_ptr<contained_type>;

private:
    shared_type m_shared{};

public:
    explicit MmQtHandle(std::nullptr_t) {}
    explicit MmQtHandle(const shared_type &event)
        /* throws invalid argument */
        noexcept(false)
        : m_shared{event}
    {
        requireValid(); /* throws invalid argument */
    }

public:
    explicit MmQtHandle() = default; /* required by QT */
    DEFAULT_RULE_OF_5(MmQtHandle);

public:
    // keep as non-inline for debugging
    bool isValid() const { return m_shared != nullptr; }
    inline explicit operator bool() const { return isValid(); }

public:
    inline bool operator==(std::nullptr_t) const { return m_shared == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return m_shared != nullptr; }

public:
    inline bool operator==(const MmQtHandle &rhs) const { return m_shared == rhs.m_shared; }
    inline bool operator!=(const MmQtHandle &rhs) const { return !(*this == rhs); }

public:
    inline const MmQtHandle &requireValid() const noexcept(false)
    {
        if (!isValid())
            throw NullPointerException();

        return *this;
    }

public:
    const shared_type &getShared() const noexcept(false)
    {
        if (auto &p = m_shared)
            return p;
        throw NullPointerException();
    }

public:
    contained_type &deref() const
        /* throws NullPointerException */
        noexcept(false)
    {
        return ::deref(m_shared);
    }
};

#endif // MMAPPER_MMQTHANDLE_H
