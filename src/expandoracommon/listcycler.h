#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef LISTCYCLER
#define LISTCYCLER

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

#endif
