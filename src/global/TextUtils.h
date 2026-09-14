#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"
#include "utils.h"

#include <cctype>
#include <functional>
#include <string>
#include <string_view>

#include <QString>

extern void trim_newline_inplace(std::string_view &sv);

NODISCARD extern bool isAbbrev(std::string_view abbr, std::string_view fullText);

namespace text_utils {

template<typename T>
struct NODISCARD SplitResult final
{
    T left{};
    T right{};
};

NODISCARD SplitResult<std::string_view> split_at(std::string_view sv, size_t pos);

NODISCARD std::string_view take_prefix(std::string_view &sv, size_t length);
NODISCARD std::string_view take_suffix(std::string_view &sv, size_t length);

template<typename Callback>
NODISCARD size_t measure_prefix_matching(std::string_view sv, Callback &&callback)
{
    size_t n = 0;
    for (size_t size = sv.size(); n < size && callback(sv[n]);) {
        ++n;
    }
    return n;
}

template<typename Callback>
NODISCARD size_t measure_suffix_matching(std::string_view sv, Callback &&callback)
{
    size_t n = 0;
    for (size_t i = sv.size(); i-- > 0 && callback(sv[i]);) {
        ++n;
    }
    return n;
}

template<typename Callback>
NODISCARD std::string_view take_prefix_matching(std::string_view &sv, Callback &&callback)
{
    const auto len = measure_prefix_matching(sv, std::forward<Callback>(callback));
    return take_prefix(sv, len);
}

template<typename Callback>
NODISCARD std::string_view take_suffix_matching(std::string_view &sv, Callback &&callback)
{
    const auto len = measure_suffix_matching(sv, std::forward<Callback>(callback));
    return take_suffix(sv, len);
}

} // namespace text_utils

namespace mmqt {
NODISCARD extern int findTrailingWhitespace(QStringView line);
NODISCARD extern int findTrailingWhitespace(const QString &line);
NODISCARD extern QString toQStringLatin1(std::string_view sv);
NODISCARD extern QString toQStringUtf8(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayRaw(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayLatin1(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayUtf8(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayLatin1(const QString &qs);
NODISCARD extern std::string toStdStringLatin1(const QString &qs);
NODISCARD extern std::string toStdStringUtf8(const QString &qs);
NODISCARD extern std::string_view toStdStringViewLatin1(const QByteArray &arr);
NODISCARD extern std::string_view toStdStringViewRaw(const QByteArray &arr);
NODISCARD extern std::string_view toStdStringViewUtf8(const QByteArray &arr);

void foreach_regex(const QRegularExpression &regex,
                   const QStringView text,
                   const std::function<void(const QStringView match)> &callback_match,
                   const std::function<void(const QStringView between)> &callback_between);
} // namespace mmqt

namespace test {
extern void testTextUtils();
} // namespace test
