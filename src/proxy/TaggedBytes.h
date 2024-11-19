#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/macros.h"
#include "../global/utils.h"

#include <QByteArray>
#include <QDebug>

// clang-format off
#define XFOREACH_TAGGED_BYTE_TYPES(X) \
    X(Ascii)                \
    X(Latin1)               \
    X(Raw)                  \
    X(RemoteEditMessage)    \
    X(Secret)               \
    X(TelnetCharset)        \
    X(TelnetIac)            \
    X(TelnetMssp)           \
    X(TelnetTermType)       \
    X(Utf8)
// clang-format on

namespace tags {
#define X_DECL_TAG(name) \
    struct NODISCARD Tag##name##Bytes final \
    {};
XFOREACH_TAGGED_BYTE_TYPES(X_DECL_TAG)
#undef X_DECL_TAG
} // namespace tags

template<typename Tag>
struct NODISCARD TaggedBytes : private QByteArray
{
public:
    TaggedBytes() = default;
    explicit TaggedBytes(QByteArray arr)
        : QByteArray{std::move(arr)}
    {}
    explicit TaggedBytes(const char *const s)
        : QByteArray{&deref(s)} // can throw if s is nullptr
    {}

    TaggedBytes(QString) = delete;
    TaggedBytes(std::string) = delete;
    TaggedBytes(std::nullptr_t) = delete;

    TaggedBytes &operator=(QString) = delete;
    TaggedBytes &operator=(QByteArray) = delete;
    TaggedBytes &operator=(std::string) = delete;
    TaggedBytes &operator=(const char *) = delete;
    TaggedBytes &operator=(std::nullptr_t) = delete;

public:
    NODISCARD QByteArray &getQByteArray() { return *this; }
    NODISCARD const QByteArray &getQByteArray() const { return *this; }

    NODISCARD auto begin() const { return QByteArray::begin(); }
    NODISCARD auto end() const { return QByteArray::end(); }

public:
    using QByteArray::append;
    using QByteArray::at;
    using QByteArray::back;
    using QByteArray::clear;
    using QByteArray::endsWith;
    using QByteArray::isEmpty;
    using QByteArray::length;
    using QByteArray::size;
    using QByteArray::startsWith;
    using QByteArray::operator[];
    using QByteArray::operator+=;

public:
    NODISCARD friend bool operator==(const TaggedBytes &a, const TaggedBytes &b)
    {
        return a.getQByteArray() == b.getQByteArray();
    }
    NODISCARD friend bool operator!=(const TaggedBytes &a, const TaggedBytes &b)
    {
        return !(a == b);
    }
    friend QDebug operator<<(QDebug debug, const TaggedBytes &tagged)
    {
        return debug << tagged.getQByteArray();
    }
};

// These are registered in metatypes.cpp

#define X_DECL_TAGGED_BYTES(name) using name##Bytes = TaggedBytes<tags::Tag##name##Bytes>;
XFOREACH_TAGGED_BYTE_TYPES(X_DECL_TAGGED_BYTES)
#undef X_DECL_TAGGED_BYTES

using GroupSecret = SecretBytes;
