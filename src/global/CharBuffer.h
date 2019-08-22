#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
}
