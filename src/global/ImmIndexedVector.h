#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "macros.h"

#include <cassert>
#include <cstddef>
#include <vector>

#include <immer/algorithm.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

template<typename T, typename Index_>
struct NODISCARD ImmIndexedVector final
{
private:
    using Base = immer::vector<T>;
    Base m_vec;

public:
    using Index = Index_;

public:
    void init(const T *const data, const size_t size)
    {
        m_vec = Base{data, data + size};
        assert(m_vec.size() == size);
    }

public:
    NODISCARD auto begin() const { return m_vec.begin(); }
    NODISCARD auto end() const { return m_vec.end(); }
    void clear() { m_vec = std::move(m_vec).clear(); }
    NODISCARD bool empty() const { return m_vec.empty(); }
    // REVISIT: immer has no equivalent reserve()
    void resize(size_t cap)
    {
        const auto old = m_vec.size();
        if (old == cap) {
            return;
        }

        auto t = std::move(m_vec).transient();
        if (t.size() > cap) {
            t.take(cap);
        } else {
            while (t.size() < cap) {
                t.push_back(T{});
            }
        }
        m_vec = std::move(t).persistent();
    }
    NODISCARD size_t size() const { return m_vec.size(); }

public:
    template<typename Callback>
    void for_each(Callback &&callback) const
    {
        immer::for_each_chunk(m_vec, [&callback](const auto *begin, const auto *end) {
            for (const auto *it = begin; it != end; ++it) {
                callback(*it);
            }
        });
    }

private:
    NODISCARD static size_t index(const Index e) { return static_cast<size_t>(e.value()); }
    NODISCARD const T &at(const Index e) const { return m_vec.at(index(e)); }

public:
    NODISCARD const T &operator[](const Index e) const { return at(e); }

public:
    void growToInclude(const Index e)
    {
        const size_t i = index(e);
        if (i >= size()) {
            resize(i + 1);
        }
    }

public:
    void set(const Index e, T x)
    {
        growToInclude(e);
        m_vec = std::move(m_vec).set(index(e), std::move(x));
    }
    template<typename Callback>
    void update(const Index e, Callback &&callback)
    {
        m_vec = std::move(m_vec).update(index(e), [&callback](T tmp) -> T {
            callback(tmp);
            return tmp;
        });
    }

    void push_back(T x) { m_vec = std::move(m_vec).push_back(std::move(x)); }

public:
    NODISCARD const T *find(const Index e) const
    {
        const auto i = index(e);
        if (i >= size()) {
            return nullptr;
        }
        return &m_vec.at(i);
    }

public:
    NODISCARD static bool areEquivalent(const ImmIndexedVector &va, const ImmIndexedVector &vb)
    {
        if (va.size() > vb.size()) {
            return areEquivalent(vb, va); // swap order
        }

        const Base &a = va.m_vec;
        const Base &b = vb.m_vec;

        const size_t minSize = va.size();
        for (size_t i = 0; i < minSize; ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }

        const size_t maxSize = vb.size();
        assert(minSize <= maxSize);
        for (size_t i = minSize; i < maxSize; ++i) {
            if (b[i] != T{}) {
                return false;
            }
        }

        return true;
    }

public:
    NODISCARD bool operator==(const ImmIndexedVector &rhs) const
    {
        return areEquivalent(*this, rhs);
    }
    NODISCARD bool operator!=(const ImmIndexedVector &rhs) const { return !(rhs == *this); }
};
