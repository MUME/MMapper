#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "macros.h"

#include <array>
#include <stdexcept>
#include <vector>

template<typename T>
struct NODISCARD View
{
private:
    const T *m_ptr = nullptr;
    size_t m_size = 0;

public:
    using reference = const T &;
    using pointer = const T *;
    using size_type = size_t;

public:
    View() = default;

    explicit View(const T *const ptr, const size_t size)
        : m_ptr{ptr}
        , m_size{size}
    {
        if (ptr == nullptr && size != 0) {
            throw std::invalid_argument("ptr");
        }
    }

    IMPLICIT View(const std::vector<T> &v)
        : View{v.data(), v.size()}
    {}

    template<size_t N>
    IMPLICIT View(const std::array<T, N> &v)
        : View{v.data(), v.size()}
    {}

public:
    NODISCARD auto begin() const { return data(); }
    NODISCARD auto end() const { return begin() + size(); }
    NODISCARD bool empty() const { return size() == 0; }
    NODISCARD pointer data() const { return m_ptr; }
    NODISCARD size_type size() const { return m_size; }

public:
    NODISCARD reference at(const size_type pos) const
    {
        if (!(pos < size())) {
            throw std::out_of_range("pos");
        }
        return m_ptr[pos];
    }
    NODISCARD reference operator[](const size_type pos) const { return at(pos); }
};

template<typename T>
View(const std::vector<T> &) -> View<T>;

template<typename T, size_t N>
View(const std::array<T, N> &) -> View<T>;
