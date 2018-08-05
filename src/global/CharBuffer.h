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

#ifndef MMAPPER_CHARBUFFER_H
#define MMAPPER_CHARBUFFER_H

#include <cstddef>
#include <cstring>

template<size_t N>
struct CharBuffer
{
protected:
    char buffer[N + 1];

public:
    explicit CharBuffer() = delete;
    explicit CharBuffer(const char (&data)[N])
        : buffer{}
    {
        auto &buf = this->buffer;
        std::memcpy(buf, data, N);
        buf[N] = '\0';
    }

public:
    const char *getBuffer() noexcept { return buffer; }
    explicit operator const char *() noexcept { return getBuffer(); }

public:
    int size() const noexcept { return N; }
    char *begin() noexcept { return buffer; }
    char *end() noexcept { return begin() + size(); }
    const char *begin() const noexcept { return buffer; }
    const char *end() const noexcept { return begin() + size(); }

public:
    void replaceAll(char from, char to)
    {
        for (auto &x : *this)
            if (x == from)
                x = to;
    }
};

template<size_t N>
CharBuffer<N> makeCharBuffer(const char (&data)[N])
{
    return CharBuffer<N>{data};
};

#endif //MMAPPER_CHARBUFFER_H
