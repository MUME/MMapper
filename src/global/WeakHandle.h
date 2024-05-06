#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "utils.h"

#include <memory>
#include <optional>

/// The object's acceptVisitor() allows safe use the object if it still exists.
/// However since no access is given to the underlying weak_ptr, clients cannot
/// extend the lifetime of the object beyond the call to acceptVisitor().
///
/// A <tt>WeakHandle&lt;T&gt;</tt> can be acquired by:<ul>
/// <li>copying another <tt>WeakHandle</tt></li>
///
/// <li>calling <tt>o.getWeakHandleFromThis()</tt> on an object <tt>o</tt> of type
///     <tt>T : public EnableGetWeakHandleFromThis&lt;T&gt;</tt>, or</li>
///
/// <li>by accessing a child <tt>WeakHandleLifetime&lt;T&gt;</tt>'s <tt>child.getWeakHandle()</tt>
///     member function.</li>
/// </ul>
template<typename T>
class NODISCARD WeakHandle final
{
private:
    std::weak_ptr<T> m_weakPtr;

public:
    WeakHandle() = default;
    explicit WeakHandle(const std::weak_ptr<T> &weak)
        : m_weakPtr(weak)
    {
        static_assert(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_array_v<T>);
    }

public:
    NODISCARD WeakHandle<const T> asConst() const
    {
        if constexpr (std::is_const_v<T>)
            return *this;
        return WeakHandle<const T>(m_weakPtr);
    }
    explicit operator WeakHandle<const T>() const { return asConst(); }

public:
    template<typename Visitor>
    bool acceptVisitor(Visitor &&visitor) const
    {
        if (auto shared = m_weakPtr.lock()) {
            T &ref = *shared;
            visitor(ref);
            return true;
        }
        return false;
    }
};

/// \see WeakHandle
template<typename T>
class NODISCARD EnableGetWeakHandleFromThis
{
private:
    const std::shared_ptr<char> m_dummy;

public:
    EnableGetWeakHandleFromThis()
        : m_dummy(std::make_shared<char>('\0'))
    {
        static_assert(std::is_base_of_v<EnableGetWeakHandleFromThis<T>, T>);
    }

public:
    NODISCARD WeakHandle<T> getWeakHandleFromThis()
    {
        return WeakHandle<T>{std::shared_ptr<T>(m_dummy, static_cast<T *>(this))};
    }
    NODISCARD WeakHandle<const T> getWeakHandleFromThis() const
    {
        return WeakHandle<const T>{std::shared_ptr<const T>(m_dummy, static_cast<const T *>(this))};
    }

public:
    DELETE_CTORS_AND_ASSIGN_OPS(EnableGetWeakHandleFromThis);
    ~EnableGetWeakHandleFromThis() = default;
};

/// \see WeakHandle
template<typename T>
class NODISCARD WeakHandleLifetime final
{
private:
    std::shared_ptr<char> m_dummy;
    T &m_parent;

public:
    explicit WeakHandleLifetime(T &parent)
        : m_dummy(std::make_shared<char>('\0'))
        , m_parent(parent)
    {
        const auto parent_beg = reinterpret_cast<uintptr_t>(&parent);
        const auto parent_end = reinterpret_cast<uintptr_t>(&parent + 1);
        const auto this_beg = reinterpret_cast<uintptr_t>(this);
        const auto this_end = reinterpret_cast<uintptr_t>(this + 1);

        if (this_beg < parent_beg || this_end > parent_end)
            throw std::runtime_error("must be a direct child member parent");
    }

public:
    NODISCARD WeakHandle<T> getWeakHandle()
    {
        return WeakHandle<T>{std::shared_ptr<T>(m_dummy, &m_parent)};
    }

    NODISCARD WeakHandle<const T> getWeakHandle() const
    {
        return WeakHandle<const T>{std::shared_ptr<const T>(m_dummy, &m_parent)};
    }

public:
    WeakHandleLifetime() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(WeakHandleLifetime);
    ~WeakHandleLifetime() = default;
};
