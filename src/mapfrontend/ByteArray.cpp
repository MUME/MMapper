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

#include "ByteArray.h"

#include <cassert>
#include <cstring>

#include "../global/utils.h"

ByteArray::ByteArray(const char *s)
    : ByteArray{s, s ? std::strlen(s) : 0u}
{
    deref(s);
}

ByteArray::ByteArray(const char *const s, const size_t len)
    : base{&deref(s), s + len}
{
    assert(size() == len);
}

static const char *maybeDeref(const char *const begin, const char *const end)
{
    if (begin == end)
        return begin;

    deref(begin);
    return begin;
}

ByteArray::ByteArray(const char *const begin_, const char *const end_)
    : base{maybeDeref(begin_, end_), end_}
{
    assert(size() == static_cast<size_t>(end_ - begin_));
}
ByteArray::ByteArray(const base::const_iterator &begin_, const base::const_iterator &end_)
    : base{begin_, end_}
{
    assert(size() == static_cast<size_t>(end_ - begin_));
}

ByteArray ByteArray::skip(const size_t count) const
{
    const auto avail = size();
    if (count >= avail) {
        return ByteArray{};
    }
    return ByteArray{begin() + count, end()};
}

void ByteArray::append(const char *const string)
{
    deref(string);
    const size_t len = std::strlen(string);
    insert(end(), string, string + len);
}
void ByteArray::append(const std::string &string)
{
    auto data = string.data();
    insert(end(), data, data + string.size());
}
