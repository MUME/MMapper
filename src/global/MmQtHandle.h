#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "NullPointerException.h"
#include "RuleOf5.h"
#include "utils.h"

#include <cstddef>
#include <memory>

/// Handle used for QT signals.
///
/// This is effectively just a thin wrapper around shared_ptr<T>
/// that helps guarantee the shared_ptr can never be nullptr.
///
/// NOTE: This class purposely avoids wrapping shared_ptr's operator->(),
/// operator*(), reset(), etc.
template<typename T>
class NODISCARD MmQtHandle final
{
public:
    using contained_type = T;
    using shared_type = std::shared_ptr<contained_type>;

private:
    shared_type m_shared;

public:
    explicit MmQtHandle(std::nullptr_t) {}
    explicit MmQtHandle(const shared_type &event)
        /* throws NullPointerException */
        noexcept(false)
        : m_shared{event}
    {
        requireValid(); /* throws NullPointerException */
    }

public:
    MmQtHandle() = default; /* required by QT */
    DEFAULT_RULE_OF_5(MmQtHandle);

public:
    // keep as non-inline for debugging
    NODISCARD bool isValid() const { return m_shared != nullptr; }
    NODISCARD inline explicit operator bool() const { return isValid(); }

public:
    NODISCARD inline bool operator==(std::nullptr_t) const { return m_shared == nullptr; }
    NODISCARD inline bool operator!=(std::nullptr_t) const { return m_shared != nullptr; }

public:
    NODISCARD inline bool operator==(const MmQtHandle &rhs) const
    {
        return m_shared == rhs.m_shared;
    }
    NODISCARD inline bool operator!=(const MmQtHandle &rhs) const { return !(*this == rhs); }

public:
    /* result can be discarded; throws NullPointerException */
    inline const MmQtHandle &requireValid() const noexcept(false)
    {
        if (!isValid())
            throw NullPointerException();

        return *this;
    }

public:
    NODISCARD const shared_type &getShared() const noexcept(false)
    {
        if (auto &p = m_shared)
            return p;
        throw NullPointerException();
    }

public:
    NODISCARD contained_type &deref() const
        /* throws NullPointerException */
        noexcept(false)
    {
        return ::deref(m_shared);
    }
};
