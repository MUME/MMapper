#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "utils.h"

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

#include <QByteArray>
#include <QChar>
#include <QString>

/*
 * NOTE: As a view, it requires the string object to remain valid and unchanged for the entire lifetime.
 *
 * NOTE: This is does not fill the same role as QStringView or std::string_view.
 */
struct NODISCARD StringView final
{
public:
    using const_iterator = std::string_view::const_iterator;

private:
    std::string_view m_sv;

public:
    StringView() noexcept = default;
    explicit StringView(std::string_view sv) noexcept;
    explicit StringView(const std::string &s) noexcept;
    explicit StringView(std::string &&) = delete;
    explicit StringView(const QString &) = delete;

public:
    NODISCARD const_iterator begin() const noexcept { return m_sv.begin(); }
    NODISCARD const_iterator end() const noexcept { return m_sv.end(); }
    NODISCARD size_t size() const noexcept { return m_sv.size(); }
    NODISCARD size_t length() const noexcept { return m_sv.length(); }
    NODISCARD bool isEmpty() const noexcept { return empty(); }
    NODISCARD bool empty() const noexcept { return m_sv.empty(); }

public:
    NODISCARD std::string toStdString() const;
    NODISCARD QString toQString() const;
    NODISCARD QByteArray toQByteArray() const;
    NODISCARD std::string_view getStdStringView() const { return m_sv; }

public:
    // allow discard
    StringView &trimLeft() noexcept;
    // allow discard
    StringView &trimRight() noexcept;
    // allow discard
    StringView &trim() noexcept { return trimLeft().trimRight(); }

private:
    void mustNotBeEmpty() const;
    void eatFirst();
    void eatLast();

public:
    NODISCARD char firstChar() const;
    NODISCARD char lastChar() const;

public:
    NODISCARD char takeFirstLetter();

public:
    NODISCARD StringView takeFirstWord();
    NODISCARD StringView takeFirstWordNoPostTrim();

public:
    NODISCARD int countNonSpaceChars() const noexcept;
    NODISCARD int countWords() const;
    NODISCARD std::vector<StringView> getWords() const;
    NODISCARD std::vector<std::string> getWordsAsStdStrings() const;
    NODISCARD std::vector<QString> getWordsAsQStrings() const;

public:
    NODISCARD bool operator==(const char *s) const noexcept = delete;

    NODISCARD bool operator==(const std::string_view sv) const noexcept { return m_sv == sv; }
    NODISCARD bool operator!=(const std::string_view sv) const noexcept { return !(*this == sv); }

    NODISCARD bool operator==(const StringView rhs) const noexcept { return m_sv == rhs.m_sv; }
    NODISCARD bool operator!=(const StringView rhs) const noexcept { return !(*this == rhs); }

public:
    NODISCARD StringView substr(size_t pos, size_t len = std::string_view::npos) const;
    // std::string s = "LeftIgnored";
    // StringView sv{s};
    // assert(sv.left(4).toStdString() == "Left");
    NODISCARD StringView left(size_t len) const;
    // std::string s = "IgnoredMid";
    // StringView sv{s};
    // assert(sv.mid(7).toStdString() == "Mid");
    NODISCARD StringView mid(size_t pos) const;
    // std::string s = "RmidIgnored";
    // StringView sv{s};
    // assert(sv.rmid(7).toStdString() == "Rmid");
    NODISCARD StringView rmid(size_t pos) const;
    // std::string s = "IgnoredRight";
    // StringView sv{s};
    // assert(sv.right(5).toStdString() == "Right");
    NODISCARD StringView right(size_t len) const;
    ALLOW_DISCARD StringView &operator++();
    void operator++(int) = delete;
    NODISCARD char operator[](size_t pos) const;
    NODISCARD bool startsWith(std::string_view other) const;
    NODISCARD bool endsWith(std::string_view other) const;
    void remove_suffix(size_t n);

public:
    // This O(1) function returns true if it intersects the other string, with a special case for empty strings,
    // with the net result being that sv intersects any of its possible substrings, but two adjacent
    // non-zero-length substrings do not intersect.
    //
    // <ecode>
    // {
    //     const std::string s = "test";
    //     const StringView sv{s};
    //     assert(sv.intersects(sv.left(0)));      // "test" vs "" (at position 0)
    //     assert(sv.intersects(sv.substr(2, 0))); // "test" vs "" (at position 2)
    //     assert(sv.intersects(sv.right(0)));     // "test" vs "" (at position 4)
    //     //
    //     assert(sv.left(3).intersects(sv.right(2)));  // "tes" vs "st"
    //     assert(sv.left(2).intersects(sv.right(3)));  // "te" vs "est"
    //     assert(!sv.left(2).intersects(sv.right(2))); // "te" vs "st" does not intersect
    //     //
    //     assert(sv.left(0).intersects(sv.mid(0)));  // same as sv.intersects(sv.left(0));
    //     assert(!sv.left(1).intersects(sv.mid(1))); // "t" vs "est" does not intersect
    //     assert(!sv.left(2).intersects(sv.mid(2))); // "te" vs "st" does not intersect
    //     assert(!sv.left(3).intersects(sv.mid(3))); // "tes" vs "t" does not intersect
    //     assert(sv.left(4).intersects(sv.mid(4)));  // same as sv.intersects(sv.right(0));
    // }
    // </ecode>
    NODISCARD bool intersects(StringView other) const;

    // This O(1) function returns if this string points to an actual substring of the other string.
    // NOTE: This function uses comparison of pointers, so it is not to be confused with the O(M+N)
    // test of whether or not `other.find(*this) != npos`.
    NODISCARD bool isSubstringOf(StringView other) const;

public:
    // std::string s = "ResultOtherIgnored";
    // StringView sv{s};
    // assert(sv.beforeSubstring(sv.mid(6, 5)/* "Other" */) == sv.left(6) /* "Result" */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    NODISCARD StringView beforeSubstring(StringView other) const;

    // std::string s ="IgnoredOtherResult";
    // StringView sv{};
    // assert(sv.startingWithSubstring(sv.mid(7, 5) /* "Other" */) == sv.mid(7) /* "OtherResult" */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    NODISCARD StringView startingWithSubstring(StringView other) const;

    // std::string s = "ResultOtherIgnored";
    // StringView sv{s};
    // assert(sv.beforeSubstring(sv.mid(6, 5) /* "Other" */) == sv.left(11) /* "ResultOther" */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    NODISCARD StringView upToAndIncludingSubstring(StringView other) const;

    // std::string s = "IgnoredOtherResult";
    // StringView sv{s};
    // assert(sv.afterSubstring(sv.substr(7, 5) /* "Other */) == sv.right(6) /* Result */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    NODISCARD StringView afterSubstring(StringView other) const;
};

namespace test {
void testStringView();
} // namespace test
