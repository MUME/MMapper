// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "TextUtils.h"

#include "Charset.h"
#include "Consts.h"
#include "tests.h"

#include <cstring>
#include <iostream>

#include <QRegularExpression>
#include <QString>

namespace { // anonymous

void maybe_remove_suffix_inplace(std::string_view &sv, const char c)
{
    if (!sv.empty() && sv.back() == c) {
        sv.remove_suffix(1);
    }
}

} // namespace

void trim_newline_inplace(std::string_view &sv)
{
    maybe_remove_suffix_inplace(sv, char_consts::C_NEWLINE);
    maybe_remove_suffix_inplace(sv, char_consts::C_CARRIAGE_RETURN);
}

bool isAbbrev(const std::string_view abbr, const std::string_view fullText)
{
    return !abbr.empty()                               //
           && abbr.size() <= fullText.size()           //
           && abbr == fullText.substr(0, abbr.size()); //
}

namespace text_utils {
SplitResult<std::string_view> split_at(const std::string_view sv, const size_t pos)
{
    assert(pos <= sv.size());
    return {sv.substr(0, pos), sv.substr(pos, sv.size() - pos)};
}

std::string_view take_prefix(std::string_view &sv, const size_t length)
{
    auto tmp = split_at(sv, length);
    sv = tmp.right;
    return tmp.left;
}

std::string_view take_suffix(std::string_view &sv, const size_t length)
{
    auto tmp = split_at(sv, sv.size() - length);
    sv = tmp.left;
    return tmp.right;
}

} // namespace text_utils

namespace mmqt {
int findTrailingWhitespace(const QStringView line)
{
    static const QRegularExpression trailingWhitespaceRegex(R"([[:space:]]+$)");
    auto m = trailingWhitespaceRegex.match(line);
    if (!m.hasMatch()) {
        return -1;
    }
    return static_cast<int>(m.capturedStart());
}

int findTrailingWhitespace(const QString &line)
{
    return findTrailingWhitespace(QStringView{line});
}

QString toQStringLatin1(const std::string_view sv)
{
    return QString::fromLatin1(sv.data(), static_cast<int>(sv.size()));
}

QString toQStringUtf8(const std::string_view sv)
{
    return QString::fromUtf8(sv.data(), static_cast<int>(sv.size()));
}

// we expect input to be in Latin1 or Utf8
QByteArray toQByteArrayRaw(const std::string_view sv)
{
    return QByteArray{sv.data(), static_cast<int>(sv.size())};
}

QByteArray toQByteArrayLatin1(const std::string_view sv)
{
    return toQByteArrayRaw(sv);
}

QByteArray toQByteArrayUtf8(const std::string_view sv)
{
    return toQByteArrayRaw(sv);
}

QByteArray toQByteArrayLatin1(const QString &input_qs)
{
    auto qs = input_qs;
    mmqt::toLatin1InPlace(qs);
    return qs.toLatin1();
}

std::string toStdStringLatin1(const QString &qs)
{
    return toQByteArrayLatin1(qs).toStdString();
}

std::string toStdStringUtf8(const QString &qs)
{
    return qs.toUtf8().toStdString();
}

std::string_view toStdStringViewRaw(const QByteArray &arr)
{
    return std::string_view{arr.data(), static_cast<size_t>(arr.size())};
}

std::string_view toStdStringViewLatin1(const QByteArray &arr)
{
    return toStdStringViewRaw(arr);
}

std::string_view toStdStringViewUtf8(const QByteArray &arr)
{
    return toStdStringViewRaw(arr);
}

void foreach_regex(const QRegularExpression &regex,
                   const QStringView text,
                   const std::function<void(const QStringView match)> &callback_match,
                   const std::function<void(const QStringView between)> &callback_between)
{
    auto it = regex.globalMatch(text);
    qsizetype pos = 0;
    auto end = text.length();
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        const auto match_start = match.capturedStart(0);
        const auto match_end = match.capturedEnd(0);
        if (match_start != pos) {
            callback_between(text.mid(pos, match_start - pos));
        }
        callback_match(text.mid(match_start, match_end - match_start));
        pos = match_end;
    }
    if (pos != end) {
        callback_between(text.mid(pos));
    }
}

} // namespace mmqt

namespace { // anonymous

void testPrefixSuffix()
{
    using ascii::isLower;
    using ascii::isUpper;

    using namespace text_utils;
    const std::string_view sv_leftRIGHT = "leftRIGHT";
    TEST_ASSERT(split_at(sv_leftRIGHT, 4).left == "left");
    TEST_ASSERT(split_at(sv_leftRIGHT, 4).right == "RIGHT");

    TEST_ASSERT(measure_prefix_matching(sv_leftRIGHT, isUpper) == 0);
    TEST_ASSERT(measure_prefix_matching(sv_leftRIGHT, isLower) == 4);
    TEST_ASSERT(measure_suffix_matching(sv_leftRIGHT, isUpper) == 5);
    TEST_ASSERT(measure_suffix_matching(sv_leftRIGHT, isLower) == 0);

    {
        auto copy = sv_leftRIGHT;
        TEST_ASSERT(take_prefix(copy, 4) == "left");
        TEST_ASSERT(copy == "RIGHT");
    }

    {
        auto copy = sv_leftRIGHT;
        TEST_ASSERT(take_suffix(copy, 5) == "RIGHT");
        TEST_ASSERT(copy == "left");
    }

    {
        auto copy = sv_leftRIGHT;
        TEST_ASSERT(take_prefix_matching(copy, isLower) == "left");
        TEST_ASSERT(copy == "RIGHT");
    }

    {
        auto copy = sv_leftRIGHT;
        TEST_ASSERT(take_suffix_matching(copy, isUpper) == "RIGHT");
        TEST_ASSERT(copy == "left");
    }
}

void testTrim()
{
    {
        std::string_view copy = string_consts::SV_NEWLINE;
        trim_newline_inplace(copy);
        TEST_ASSERT(copy.empty());
    }
    {
        std::string_view copy = "abc\n";
        trim_newline_inplace(copy);
        TEST_ASSERT(copy == "abc");
        trim_newline_inplace(copy);
        TEST_ASSERT(copy == "abc");
    }
}

} // namespace

namespace test {
void testTextUtils()
{
    testPrefixSuffix();
    testTrim();
}
} // namespace test
