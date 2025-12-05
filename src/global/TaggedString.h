#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Consts.h"
#include "NullPointerException.h"
#include "RuleOf5.h"
#include "TextUtils.h"

#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#include <QByteArray>
#include <QString>

// TaggedStringUtf8 is always Utf8.
template<typename Tag_>
class NODISCARD TaggedStringUtf8
{
public:
    using Tag = Tag_;

private:
    std::string m_str;

public:
    TaggedStringUtf8() = default;
    DEFAULT_RULE_OF_5(TaggedStringUtf8);

public:
    explicit TaggedStringUtf8(std::string s)
        : m_str(std::move(s))
    {
        if (!Tag::isValid(std::string_view{m_str})) {
            throw std::runtime_error("invalid input");
        }
    }

public:
    explicit TaggedStringUtf8(std::nullptr_t) = delete;
    explicit TaggedStringUtf8(const char *const s)
        : TaggedStringUtf8{std::string{(s == nullptr) ? "" : s}}
    {
        assert(s != nullptr);
    }
    template<size_t N>
    explicit TaggedStringUtf8(const char (&s)[N])
        : TaggedStringUtf8{std::string{s}}
    {}
    explicit TaggedStringUtf8(const QString &s)
        : TaggedStringUtf8(mmqt::toStdStringUtf8(s))
    {}

public:
    NODISCARD std::string_view getStdStringViewUtf8() const & { return m_str; }

public:
    NODISCARD const std::string &getStdStringUtf8() const & { return m_str; }
    NODISCARD std::string getStdStringUtf8() && { return std::exchange(m_str, {}); }

public:
    NODISCARD bool empty() const { return m_str.empty(); }
    NODISCARD bool isEmpty() const { return empty(); }

public:
    NODISCARD bool operator==(const TaggedStringUtf8 &rhs) const { return m_str == rhs.m_str; }
    NODISCARD bool operator!=(const TaggedStringUtf8 &rhs) const { return !(rhs == *this); }

public:
    NODISCARD QByteArray toQByteArray() const { return mmqt::toQByteArrayUtf8(getStdStringUtf8()); }
    NODISCARD QString toQString() const { return mmqt::toQStringUtf8(getStdStringUtf8()); }
};

// TaggedStringUtf8 is always Utf8;
// A more appropriate name might be TaggedSharedString.
//
// Multiple different strings can point to the same storage;
// so a mutated-copy of the world can point to most of the same room descriptions.
template<typename Tag_>
class NODISCARD TaggedBoxedStringUtf8
{
public:
    using Tag = Tag_;

private:
    using SharedConstCharArray = std::shared_ptr<const std::string>;
    SharedConstCharArray m_ptr;
    std::string_view m_view;

public:
    TaggedBoxedStringUtf8() = default;
    DEFAULT_RULE_OF_5(TaggedBoxedStringUtf8);

private:
    // each tag has a distinct empty string.
    NODISCARD static SharedConstCharArray getEmptyString()
    {
        static const auto g_default = std::invoke([]() -> SharedConstCharArray {
            auto buf = std::make_shared<const std::string>();
            assert(deref(buf).empty());
            return buf;
        });
        return g_default;
    }

private:
    explicit TaggedBoxedStringUtf8(SharedConstCharArray ptr)
        : m_ptr{std::move(ptr)}
        , m_view{deref(m_ptr)}
    {
        if (m_ptr == nullptr) {
            throw NullPointerException();
        }
    }

private:
    NODISCARD static SharedConstCharArray maybeEmpty(std::string s)
    {
        if (s.empty()) {
            return getEmptyString();
        }
        return std::make_shared<const std::string>(std::move(s));
    }
    static void check(std::string_view sv)
    {
        if (!Tag::isValid(sv)) {
            throw std::runtime_error("invalid input");
        }
    }
    NODISCARD static SharedConstCharArray checkAndConstruct(std::string s)
    {
        check(s);
        return maybeEmpty(std::move(s));
    }
    NODISCARD static SharedConstCharArray checkAndConstruct(std::string_view sv)
    {
        check(sv);
        return maybeEmpty(std::string{sv});
    }

public:
    explicit TaggedBoxedStringUtf8(std::string s)
        : TaggedBoxedStringUtf8{checkAndConstruct(std::move(s))}
    {}
    explicit TaggedBoxedStringUtf8(std::string_view sv)
        : TaggedBoxedStringUtf8{checkAndConstruct(sv)}
    {}

public:
    explicit TaggedBoxedStringUtf8(std::nullptr_t) = delete;
    explicit TaggedBoxedStringUtf8(const char *const s)
        : TaggedBoxedStringUtf8{std::string_view{(s == nullptr) ? "" : s}}
    {
        if (s == nullptr) {
            throw NullPointerException();
        }
    }

public:
    template<typename U>
    explicit TaggedBoxedStringUtf8(TaggedStringUtf8<U> s)
        : TaggedBoxedStringUtf8{std::move(s).getStdStringUtf8()}
    {}

public:
    NODISCARD std::string_view getStdStringViewUtf8() const & { return m_view; }
    // NODISCARD std::string_view getStdStringViewUtf8() && = delete;
    NODISCARD std::string toStdStringUtf8() const { return std::string{m_view}; }

public:
    NODISCARD bool operator==(const TaggedBoxedStringUtf8 &rhs) const
    {
        if (m_view.size() != rhs.m_view.size()) {
            return false;
        }
        // string_view doesn't have this short circuit!
        if (reinterpret_cast<uintptr_t>(m_view.data())
            == reinterpret_cast<uintptr_t>(rhs.m_view.data())) {
            return true;
        }
        return m_view == rhs.m_view;
    }
    NODISCARD bool operator!=(const TaggedBoxedStringUtf8 &rhs) const { return !(rhs == *this); }

public:
    NODISCARD bool empty() const { return m_view.empty(); }
    NODISCARD bool isEmpty() const { return empty(); }

public:
    NODISCARD bool operator<(const TaggedBoxedStringUtf8 &rhs) const { return m_view < rhs.m_view; }
    NODISCARD bool operator>(const TaggedBoxedStringUtf8 &rhs) const { return rhs < *this; }
    NODISCARD bool operator<=(const TaggedBoxedStringUtf8 &rhs) const { return !(rhs < *this); }
    NODISCARD bool operator>=(const TaggedBoxedStringUtf8 &rhs) const { return !(*this < rhs); }

public:
    explicit TaggedBoxedStringUtf8(const QString &s)
        : TaggedBoxedStringUtf8{mmqt::toStdStringUtf8(s)}
    {}

public:
    NODISCARD QByteArray toQByteArray() const
    {
        return mmqt::toQByteArrayUtf8(getStdStringViewUtf8());
    }
    NODISCARD QString toQString() const { return mmqt::toQStringUtf8(getStdStringViewUtf8()); }
};

namespace test {
void testTaggedString();
} // namespace test
