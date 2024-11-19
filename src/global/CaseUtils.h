#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <string>

NODISCARD extern char toLowerLatin1(char c);
NODISCARD extern char toUpperLatin1(char c);
NODISCARD extern bool isLowerLatin1(std::string_view a);
NODISCARD extern bool isUpperLatin1(std::string_view a);
NODISCARD extern bool areEqualAsLowerLatin1(std::string_view a, std::string_view b);
NODISCARD extern std::string toLowerLatin1(std::string_view str);
NODISCARD extern std::string toUpperLatin1(std::string_view str);
NODISCARD extern std::string toLowerLatin1(std::string str);
NODISCARD extern std::string toUpperLatin1(std::string str);
