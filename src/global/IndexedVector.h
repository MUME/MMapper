#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <cassert>
#include <cstddef>
#include <vector>

// Expect Index_ to be a tagged int or enum.
template<typename T, typename Index_>
struct NODISCARD IndexedVector : private std::vector<T>
{
private:
    using Base = std::vector<T>;

public:
    using Index = Index_;

public:
    using Base::Base;

public:
    void init(const T *const data, const size_t size)
    {
        Base::clear();
        Base::insert(Base::end(), data, data + static_cast<ptrdiff_t>(size));
    }

public:
    using Base::begin;
    using Base::clear;
    using Base::empty;
    using Base::end;
    using Base::reserve;
    using Base::resize;
    using Base::size;

private:
    NODISCARD static size_t index(const Index e) { return static_cast<size_t>(e.value()); }

public:
    NODISCARD T at(const Index e) && { return Base::at(index(e)); }
    NODISCARD T at(const Index e) const && { return Base::at(index(e)); }
    NODISCARD T &at(const Index e) & { return Base::at(index(e)); }
    NODISCARD const T &at(const Index e) const & { return Base::at(index(e)); }

public:
    NODISCARD decltype(auto) operator[](const Index e) && { at(e); }
    NODISCARD decltype(auto) operator[](const Index e) & { return at(e); }
    NODISCARD decltype(auto) operator[](const Index e) const & { return at(e); }

public:
    void growToInclude(const Index e)
    {
        const size_t i = index(e);
        if (i >= size()) {
            Base::resize(i + 1);
        }
    }

public:
    void set(const Index e, T x)
    {
        growToInclude(e);
        at(e) = std::move(x);
    }

    void push_back(T x) { Base::push_back(std::move(x)); }

public:
    NODISCARD const T *find(const Index e) const
    {
        const auto i = index(e);
        if (i >= size())
            return nullptr;
        return Base::data() + static_cast<ptrdiff_t>(i);
    }

public:
    NODISCARD static bool areEquivalent(const IndexedVector &va, const IndexedVector &vb)
    {
        if (va.size() > vb.size())
            return areEquivalent(vb, va); // swap order

        const Base &a = va;
        const Base &b = vb;

        const size_t minSize = va.size();
        for (size_t i = 0; i < minSize; ++i) {
            if (a[i] != b[i])
                return false;
        }

        const size_t maxSize = vb.size();
        assert(minSize <= maxSize);
        for (size_t i = minSize; i < maxSize; ++i) {
            if (b[i] != T{})
                return false;
        }

        return true;
    }

public:
    NODISCARD bool operator==(const IndexedVector &rhs) const { return areEquivalent(*this, rhs); }
    NODISCARD bool operator!=(const IndexedVector &rhs) const { return !(rhs == *this); }
};
