#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Charset.h"
#include "utils.h"

#include <cassert>

#include <QString>

// MatchingRangeFn = void(string_view), is a range of contiguous C's
// NonMatchingFn = void(string_view), is a range not including C.
template<typename MatchingRangeFn, typename NonMatchingFn>
void foreachCharMulti(const std::string_view input,
                      const char c,
                      MatchingRangeFn &&matching,
                      NonMatchingFn &&nonMatching)
{
    assert(::isAscii(c));
    std::string_view sv = input;
    while (!sv.empty()) {
        if (const auto next = sv.find(c); next == std::string_view::npos) {
            nonMatching(sv);
            break;
        } else if (next > 0) {
            nonMatching(sv.substr(0, next));
            sv.remove_prefix(next);
        }

        assert(!sv.empty() && sv.front() == c);
        if (const auto span = sv.find_first_not_of(c); span == std::string_view::npos) {
            matching(sv);
            break;
        } else {
            matching(sv.substr(0, span));
            sv.remove_prefix(span);
        }
    }
}

// Callback(string_view); can contain a range of maching, or not matching.
template<typename Callback>
void foreachCharMulti2(const std::string_view input, const char c, Callback &&callback)
{
    foreachCharMulti(input, c, callback, callback);
}

namespace char_utils_detail {

// MatchingRangeFn = void() notifies the existence of char c.
// NonMatchingFn = void(string_view), is a range not including C; this is allowed to be empty.
//
// Reports leading and trailing empty non-matches.
// foreachCharSingle("", ';', ...) reports: nonmatch("")
// foreachCharSingle(";", ';', ...) reports: nonmatch(""), match(";"), nonmatch("")
template<typename MatchingCharFn, typename NonMatchingFn>
void foreachCharSingle(const std::string_view input,
                       const char c,
                       MatchingCharFn &&matching,
                       NonMatchingFn &&nonMatching)
{
    if (input.empty()) {
        nonMatching(input);
        return;
    }

    std::string_view sv = input;
    while (true) {
        assert(!sv.empty());
        const auto next = sv.find(c);
        if (next == std::string_view::npos) {
            nonMatching(sv);
            return;
        }

        /* non-matching substr is allowed to be empty */
        nonMatching(sv.substr(0, next));
        if (next > 0) {
            sv.remove_prefix(next);
        }

        matching();
        sv.remove_prefix(1);
        if (sv.empty()) {
            nonMatching(sv);
            return;
        }
    }
}

} // namespace char_utils_detail

template<typename MatchingCharFn, typename NonMatchingFn>
void foreachAsciiCharSingle(const std::string_view input,
                            const char c,
                            MatchingCharFn &&matching,
                            NonMatchingFn &&nonMatching)
{
    // Consider asserting that the input is ascii.
    assert(::isAscii(c));
    char_utils_detail::foreachCharSingle(input,
                                         c,
                                         std::forward<MatchingCharFn>(matching),
                                         std::forward<NonMatchingFn>(nonMatching));
}

// The utf8 is allowed to contain multi-byte characters that will be reported as non-matched,
// but the character must be part of the 7-bit ascii subset of utf8.
template<typename MatchingCharFn, typename NonMatchingFn>
void foreachUtf8CharSingle(const std::string_view input,
                           const char c,
                           MatchingCharFn &&matching,
                           NonMatchingFn &&nonMatching)
{
    // Consider also asserting that the input is utf8.
    assert(::isAscii(c));
    char_utils_detail::foreachCharSingle(input,
                                         c,
                                         std::forward<MatchingCharFn>(matching),
                                         std::forward<NonMatchingFn>(nonMatching));
}

// NOTE: the character is allowed to be anything for latin1 strings.
template<typename MatchingCharFn, typename NonMatchingFn>
void foreachLatin1CharSingle(const std::string_view input,
                             const char c,
                             MatchingCharFn &&matching,
                             NonMatchingFn &&nonMatching)
{
    char_utils_detail::foreachCharSingle(input,
                                         c,
                                         std::forward<MatchingCharFn>(matching),
                                         std::forward<NonMatchingFn>(nonMatching));
}

namespace mmqt {

// Callback is void(qsizetype pos)
template<typename Callback>
void foreachCharPos(const QStringView input, const char c, Callback &&callback)
{
    assert(::isAscii(c));
    const qsizetype len = input.size();
    qsizetype pos = 0;
    while (pos < len) {
        const qsizetype next = input.indexOf(c, pos);
        if (next < 0) {
            break;
        }
        assert(next >= pos);
        assert(input[next] == c);
        callback(next);
        pos = next + 1;
    }
}

template<typename Callback>
inline void foreachCharPos(const QString &input, const char c, Callback &&callback)
{
    foreachCharPos(QStringView{input}, c, std::forward<Callback>(callback));
}

} // namespace mmqt

namespace test {
extern void testCharUtils();
} // namespace test
