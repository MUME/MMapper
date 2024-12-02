#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "ConfigEnums.h"
#include "macros.h"

#include <iosfwd>
#include <ostream>
#include <string>

#include <QString>

NODISCARD extern bool isAscii(char c) noexcept;
NODISCARD extern bool isAscii(std::string_view sv);

NODISCARD extern bool isPrintLatin1(char c);

NODISCARD extern char latin1ToAscii(char c) noexcept;

extern std::string &latin1ToAsciiInPlace(std::string &str);
NODISCARD extern std::string latin1ToAscii(std::string_view sv);

namespace mmqt {
QString &toAsciiInPlace(QString &str);
}

extern void latin1ToUtf8(std::ostream &os, char c);

extern void latin1ToAscii(std::ostream &os, std::string_view sv);
extern void latin1ToUtf8(std::ostream &os, std::string_view sv);

NODISCARD extern std::string latin1ToUtf8(std::string_view sv);

// Converts input string_view sv from latin1 to the specified encoding
// by writing "raw bytes" to the output stream os.
extern void convertFromLatin1(std::ostream &os, CharacterEncodingEnum encoding, std::string_view sv);

namespace charset {
extern void utf8ToAscii(std::ostream &os, std::string_view sv);
extern void utf8ToLatin1(std::ostream &os, std::string_view sv);
NODISCARD extern std::string utf8ToLatin1(std::string_view sv);
extern void convert(std::ostream &os,
                    std::string_view sv,
                    CharacterEncodingEnum from,
                    CharacterEncodingEnum to);
} // namespace charset
