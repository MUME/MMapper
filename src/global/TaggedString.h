#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cassert>
#include <string>
#include <QByteArray>
#include <QString>

#include "RuleOf5.h"
#include "TextUtils.h"

// Latin1
template<typename T>
class TaggedString
{
private:
    std::string m_str;

public:
    TaggedString() = default;
    explicit TaggedString(std::nullptr_t) = delete;
    explicit TaggedString(const char *const s)
        : m_str((s == nullptr) ? "" : s)
    {
        assert(s != nullptr);
    }
    template<size_t N>
    explicit TaggedString(const char (&s)[N])
        : m_str(s, N)
    {
        assert(s != nullptr);
    }
    explicit TaggedString(std::string s)
        : m_str(std::move(s))
    {}
    explicit TaggedString(const QString &s)
        : m_str(::toStdStringLatin1(s))
    {}
    DEFAULT_RULE_OF_5(TaggedString);

public:
    bool operator==(const TaggedString &rhs) const { return m_str == rhs.m_str; }
    bool operator!=(const TaggedString &rhs) const { return !(rhs == *this); }

public:
    friend auto operator+(const TaggedString &taggedString, const QString &qs)
    {
        return taggedString.toQString() + qs;
    }
    friend auto operator+(const QString &qs, const TaggedString &taggedString)
    {
        return qs + taggedString.toQString();
    }

public:
    const std::string &getStdString() const { return m_str; }
    QByteArray toQByteArray() const { return ::toQByteArrayLatin1(m_str); }
    QString toQString() const { return ::toQStringLatin1(m_str); }

public:
    bool empty() const { return m_str.empty(); }
    bool isEmpty() const { return empty(); }
};

// UTF-8
template<typename T>
class TaggedStringUtf8 : private TaggedString<T>
{
private:
    using base = TaggedString<T>;

public:
    explicit TaggedStringUtf8(std::string s)
        : TaggedString<T>(std::move(s))
    {}
    explicit TaggedStringUtf8(const QString &s)
        : TaggedString<T>(::toStdStringUtf8(s))
    {}

public:
    using base::operator==;
    using base::operator!=;

public:
    friend auto operator+(const TaggedStringUtf8 &taggedString, const QString &qs)
    {
        return taggedString.toQString() + qs;
    }
    friend auto operator+(const QString &qs, const TaggedStringUtf8 &taggedString)
    {
        return qs + taggedString.toQString();
    }

public:
    using base::getStdString;
    QByteArray toQByteArray() const { return ::toQByteArrayUtf8(getStdString()); }
    QString toQString() const { return ::toQStringUtf8(getStdString()); }

public:
    using base::empty;
    using base::isEmpty;
};
