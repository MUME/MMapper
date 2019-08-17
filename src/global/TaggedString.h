#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#ifndef MMAPPER_TAGGEDSTRING_H
#define MMAPPER_TAGGEDSTRING_H

#include <cassert>
#include <string>
#include <QByteArray>
#include <QString>

#include "RuleOf5.h"

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
        : m_str(s.toLatin1().toStdString())
    {}
    DEFAULT_RULE_OF_5(TaggedString);

public:
    bool operator==(const TaggedString &rhs) const { return m_str == rhs.m_str; }
    bool operator!=(const TaggedString &rhs) const { return !(rhs == *this); }

public:
    template<typename U>
    friend auto operator+(const TaggedString &taggedString, const U &x)
    {
        return taggedString.getStdString() + x;
    }
    template<typename U>
    friend auto operator+(const U &x, const TaggedString &taggedString)
    {
        return x + taggedString.getStdString();
    }

public:
    friend auto operator+(const TaggedString &taggedString, const QString &x)
    {
        return taggedString.getStdString().c_str() + x;
    }
    friend auto operator+(const QString &x, const TaggedString &taggedString)
    {
        return x + taggedString.getStdString().c_str();
    }

public:
    const std::string &getStdString() const { return m_str; }
    QByteArray toQByteArray() const
    {
        return QByteArray{m_str.data(), static_cast<int>(m_str.size())};
    }
    QString toQString() const
    {
        return QString::fromLatin1(m_str.data(), static_cast<int>(m_str.size()));
    }

public:
    bool empty() const { return m_str.empty(); }
    bool isEmpty() const { return empty(); }
};

#endif //MMAPPER_TAGGEDSTRING_H
