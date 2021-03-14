// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "parserutils.h"

#include <iostream>
#include <stdexcept>
#include <QRegularExpression>
#include <QtCore>

namespace ParserUtils {
static constexpr const size_t IDX_NBSP = 160;
static constexpr const char LATIN1_UNDEFINED = 'z';
static constexpr const size_t NUM_ASCII_CODEPOINTS = 128;
static constexpr const size_t NUM_LATIN1_CODEPOINTS = 256;
// Taken from MUME's HELP LATIN to convert from Latin-1 to US-ASCII
static constexpr const std::array g_latin1ToAscii = {
    /*160*/
    ' ', '!', 'c', 'L', '$',  'Y', '|', 'P', '"', 'C', 'a', '<', ',', '-', 'R', '-',
    'd', '+', '2', '3', '\'', 'u', 'P', '*', ',', '1', 'o', '>', '4', '2', '3', '?',
    'A', 'A', 'A', 'A', 'A',  'A', 'A', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
    'D', 'N', 'O', 'O', 'O',  'O', 'O', '*', 'O', 'U', 'U', 'U', 'U', 'Y', 'T', 's',
    'a', 'a', 'a', 'a', 'a',  'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'd', 'n', 'o', 'o', 'o',  'o', 'o', '/', 'o', 'u', 'u', 'u', 'u', 'y', 't', 'y'};

static_assert(
    std::is_same_v<char, std::remove_const_t<std::remove_reference_t<decltype(g_latin1ToAscii[0])>>>);
static_assert(std::size(g_latin1ToAscii) == NUM_LATIN1_CODEPOINTS - IDX_NBSP);

NODISCARD static inline constexpr size_t getIndex(const char c) noexcept
{
    return static_cast<size_t>(static_cast<uint8_t>(c));
}

NODISCARD static inline constexpr bool isAscii(const char c) noexcept
{
    return getIndex(c) < NUM_ASCII_CODEPOINTS;
}

NODISCARD static inline constexpr char latin1ToAscii(char c) noexcept
{
    if (isAscii(c))
        return c;

    const auto i = getIndex(c);
    if (i >= IDX_NBSP && i < NUM_LATIN1_CODEPOINTS) {
        return g_latin1ToAscii[static_cast<size_t>(i - IDX_NBSP)];
    } else {
        return LATIN1_UNDEFINED;
    }
}

static_assert(latin1ToAscii('X') == 'X');
static_assert(latin1ToAscii('x') == 'x');
static_assert(latin1ToAscii("\x7f"[0]) == '\x7f');
static_assert(latin1ToAscii("\x80"[0]) == LATIN1_UNDEFINED);
static_assert(latin1ToAscii("\x9f"[0]) == LATIN1_UNDEFINED);
static_assert(latin1ToAscii("\xa0"[0]) == ' ');
static_assert(latin1ToAscii("\xff"[0]) == 'y');

QString &removeAnsiMarksInPlace(QString &str)
{
    static const QRegularExpression ansi("\x1b\\[[0-9;]*[A-Za-z]");
    str.remove(ansi);
    return str;
}

QString &toAsciiInPlace(QString &str)
{
    // NOTE: 128 (0x80) was not converted to 'z' before.
    for (QChar &qc : str) {
        // c++17 if statement with initializer
        if (const char ch = qc.toLatin1(); !isAscii(ch)) {
            qc = latin1ToAscii(ch);
        }
    }
    return str;
}

std::string &latin1ToAsciiInPlace(std::string &str)
{
    for (char &c : str) {
        if (!isAscii(c)) {
            c = latin1ToAscii(c);
        }
    }
    return str;
}

std::string latin1ToAscii(const std::string_view &sv)
{
    std::string tmp{sv};
    latin1ToAsciiInPlace(tmp);
    return tmp;
}

void latin1ToAscii(std::ostream &os, const std::string_view &sv)
{
    for (const char c : sv) {
        os << latin1ToAscii(c);
    }
}

} // namespace ParserUtils
