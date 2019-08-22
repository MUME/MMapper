#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <climits>
#include <cstdint>
#include <sys/types.h>

#include "../global/RuleOf5.h"

template<class T, class C>
class ListCycler : public C
{
public:
    explicit ListCycler() = default;
    DEFAULT_CTORS_AND_ASSIGN_OPS(ListCycler);

    explicit ListCycler(const C &data)
        : C(data)
        , pos(static_cast<uint32_t>(data.size()))
    {}
    virtual ~ListCycler() = default;
    virtual const T &next();
    virtual const T &prev();
    virtual const T &current();
    virtual uint32_t getPos() const { return pos; }
    virtual void reset() { pos = static_cast<uint32_t>(C::size()); }

protected:
    uint32_t pos = UINT32_MAX;
};

template<class T, class C>
const T &ListCycler<T, C>::next()
{
    static const T invalid{};
    const uint32_t nSize = static_cast<uint32_t>(C::size());

    if (pos >= nSize)
        pos = 0;
    else if (++pos == nSize)
        return invalid;

    if (pos >= nSize)
        return invalid;
    else
        return C::operator[](pos);
}

template<class T, class C>
const T &ListCycler<T, C>::prev()
{
    static const T invalid{};
    const uint32_t nSize = static_cast<uint32_t>(C::size());

    if (pos == 0) {
        pos = nSize;
        return invalid;
    } else {
        if (pos >= nSize)
            pos = nSize;
        pos--;
        return C::operator[](pos);
    }
}

template<class T, class C>
const T &ListCycler<T, C>::current()
{
    static const T invalid{};
    if (pos >= C::size())
        return invalid;
    return C::operator[](pos);
}
