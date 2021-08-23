#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstddef>
#include <cstring>

#include "macros.h"

template<size_t N>
struct NODISCARD CharBuffer
{
protected:
    char buffer[N + 1];

public:
    CharBuffer() = delete;
    explicit CharBuffer(const char (&data)[N])
        : buffer{}
    {
        auto &buf = this->buffer;
        std::memcpy(buf, data, N);
        buf[N] = '\0';
    }

public:
    NODISCARD const char *getBuffer() noexcept { return buffer; }
    NODISCARD explicit operator const char *() noexcept { return getBuffer(); }

public:
    NODISCARD int size() const noexcept { return N; }
    NODISCARD char *begin() noexcept { return buffer; }
    NODISCARD char *end() noexcept { return begin() + size(); }
    NODISCARD const char *begin() const noexcept { return buffer; }
    NODISCARD const char *end() const noexcept { return begin() + size(); }

public:
    void replaceAll(char from, char to)
    {
        for (auto &x : *this)
            if (x == from)
                x = to;
    }
};

template<size_t N>
NODISCARD static inline CharBuffer<N> makeCharBuffer(const char (&data)[N])
{
    return CharBuffer<N>{data};
}
