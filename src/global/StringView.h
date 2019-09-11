#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <QByteArray>
#include <QChar>
#include <QString>

#include "utils.h"

/*
 * NOTE: As a view, it requires the string object to remain valid and unchanged for the entire lifetime.
 *
 * NOTE: This is does not fill the same role as QStringView or std::string_view.
 */
struct StringView final
{
public:
    using const_iterator = std::string_view::const_iterator;

private:
    std::string_view m_sv;

public:
    StringView() noexcept = default;
    explicit StringView(const std::string_view &sv) noexcept;
    explicit StringView(const std::string &s) noexcept;
    explicit StringView(std::string &&) = delete;
    explicit StringView(const QString &) = delete;

public:
    inline const_iterator begin() const noexcept { return m_sv.begin(); }
    inline const_iterator end() const noexcept { return m_sv.end(); }
    inline size_t size() const noexcept { return m_sv.size(); }
    inline size_t length() const noexcept { return m_sv.length(); }
    inline bool isEmpty() const noexcept { return empty(); }
    inline bool empty() const noexcept { return m_sv.empty(); }

public:
    std::string toStdString() const noexcept(false);
    QString toQString() const noexcept(false);
    QByteArray toQByteArray() const noexcept(false);
    std::string_view getStdStringView() const { return m_sv; }

public:
    StringView &trimLeft() noexcept;
    StringView &trimRight() noexcept;
    inline StringView &trim() noexcept { return trimLeft().trimRight(); }

private:
    void mustNotBeEmpty() const noexcept(false);
    void eatFirst();
    void eatLast();

public:
    char firstChar() const noexcept(false);
    char lastChar() const noexcept(false);

public:
    char takeFirstLetter() noexcept(false);

public:
    StringView takeFirstWord() noexcept(false);
    StringView takeFirstWordNoPostTrim() noexcept(false);

public:
    int countNonSpaceChars() const noexcept;
    int countWords() const noexcept(false);
    std::vector<StringView> getWords() const noexcept(false);
    std::vector<std::string> getWordsAsStdStrings() const noexcept(false);
    std::vector<QString> getWordsAsQStrings() const noexcept(false);

public:
    DEPRECATED
    bool fuzzyEquals(const char *s) const noexcept;
    bool operator==(const char *s) const noexcept = delete;

    bool operator==(const std::string_view &sv) const noexcept { return m_sv == sv; }
    bool operator!=(const std::string_view &sv) const noexcept { return !(*this == sv); }

    bool operator==(const StringView &rhs) const noexcept { return m_sv == rhs.m_sv; }
    bool operator!=(const StringView &rhs) const noexcept { return !(*this == rhs); }

public:
    StringView substr(size_t pos, size_t len = std::string_view::npos) const;
    // std::string s = "LeftIgnored";
    // StringView sv{s};
    // assert(sv.left(4).toStdString() == "Left");
    StringView left(size_t len) const;
    // std::string s = "IgnoredMid";
    // StringView sv{s};
    // assert(sv.mid(7).toStdString() == "Mid");
    StringView mid(size_t pos) const;
    // std::string s = "RmidIgnored";
    // StringView sv{s};
    // assert(sv.rmid(7).toStdString() == "Rmid");
    StringView rmid(size_t pos) const;
    // std::string s = "IgnoredRight";
    // StringView sv{s};
    // assert(sv.right(5).toStdString() == "Right");
    StringView right(size_t len) const;
    StringView &operator++();
    StringView operator++(int);
    char operator[](size_t pos) const;
    bool startsWith(const std::string_view &other) const;
    bool endsWith(const std::string_view &other) const;
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
    bool intersects(const StringView &other) const;

    // This O(1) function returns if this string points to an actual substring of the other string.
    // NOTE: This function uses comparison of pointers, so it is not to be confused with the O(M+N)
    // test of whether or not `other.find(*this) != npos`.
    bool isSubstringOf(const StringView &other) const;

public:
    // std::string s = "ResultOtherIgnored";
    // StringView sv{s};
    // assert(sv.beforeSubstring(sv.mid(6, 5)/* "Other" */) == sv.left(6) /* "Result" */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    StringView beforeSubstring(const StringView &other) const;

    // std::string s ="IgnoredOtherResult";
    // StringView sv{};
    // assert(sv.startingWithSubstring(sv.mid(7, 5) /* "Other" */) == sv.mid(7) /* "OtherResult" */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    StringView startingWithSubstring(const StringView &other) const;

    // std::string s = "ResultOtherIgnored";
    // StringView sv{s};
    // assert(sv.beforeSubstring(sv.mid(6, 5) /* "Other" */) == sv.left(11) /* "ResultOther" */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    StringView upToAndIncludingSubstring(const StringView &other) const;

    // std::string s = "IgnoredOtherResult";
    // StringView sv{s};
    // assert(sv.afterSubstring(sv.substr(7, 5) /* "Other */) == sv.right(6) /* Result */);
    //
    // NOTE: requires `other.isSubstringOf(*this)`; result is undefined (in the UB sense) if that isn't true.
    StringView afterSubstring(const StringView &other) const;
};

namespace test {
void testStringView();
} // namespace test
