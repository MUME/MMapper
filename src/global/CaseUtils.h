#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <string>

NODISCARD extern bool isLowerLatin1(char c);
NODISCARD extern bool isUpperLatin1(char c);
NODISCARD extern char toLowerLatin1(char c);
NODISCARD extern char toUpperLatin1(char c);

NODISCARD extern bool containsLowerLatin1(std::string_view sv);
NODISCARD extern bool containsUpperLatin1(std::string_view sv);
NODISCARD extern bool areEqualAsLowerLatin1(std::string_view a, std::string_view b);

NODISCARD extern std::string toLowerLatin1(std::string_view sv);
NODISCARD extern std::string toUpperLatin1(std::string_view sv);
NODISCARD extern std::string toLowerLatin1(std::string str);
NODISCARD extern std::string toUpperLatin1(std::string str);

///

NODISCARD extern bool isLowerUtf8(char32_t codepoint);
NODISCARD extern bool isUpperUtf8(char32_t codepoint);
NODISCARD extern char32_t toLowerUtf8(char32_t codepoint);
NODISCARD extern char32_t toUpperUtf8(char32_t codepoint);

NODISCARD extern bool containsLowerUtf8(std::string_view sv);
NODISCARD extern bool containsUpperUtf8(std::string_view sv);
NODISCARD extern bool areEqualAsLowerUtf8(std::string_view a, std::string_view b);

NODISCARD extern std::string toLowerUtf8(std::string_view str);
NODISCARD extern std::string toUpperUtf8(std::string_view str);
NODISCARD extern std::string toLowerUtf8(std::string str);
NODISCARD extern std::string toUpperUtf8(std::string str);
