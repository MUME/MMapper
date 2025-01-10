// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "CaseUtils.h"

#include <cstdint>
#include <sstream>

char toLowerLatin1(const char c)
{
    const int i = static_cast<int>(static_cast<uint8_t>(c));
    if (i >= 192 && i <= 221 && i != 215) {
        // handle accented Latin-1 chars
        return static_cast<char>(i + 32);
    }
    return static_cast<char>(std::tolower(c));
}

char toUpperLatin1(const char c)
{
    const int i = static_cast<int>(static_cast<uint8_t>(c));
    if (i >= 192 && i <= 221 && i != 215) {
        // handle accented Latin-1 chars
        return static_cast<char>(i + 32);
    }
    return static_cast<char>(std::toupper(c));
}

bool isLowerLatin1(const std::string_view sv)
{
    for (char c : sv) {
        if (c != toLowerLatin1(c)) {
            return false;
        }
    }
    return true;
}

bool isUpperLatin1(const std::string_view sv)
{
    for (char c : sv) {
        if (c != toUpperLatin1(c)) {
            return false;
        }
    }
    return true;
}

bool areEqualAsLowerLatin1(const std::string_view a, const std::string_view b)
{
    const size_t size = a.size();
    if (b.size() != a.size()) {
        return false;
    }

    for (size_t i = 0; i < size; ++i) {
        if (toLowerLatin1(a[i]) != toLowerLatin1(b[i])) {
            return false;
        }
    }
    return true;
}

std::string toLowerLatin1(const std::string_view str)
{
    std::ostringstream oss;
    for (char c : str) {
        oss << toLowerLatin1(c);
    }
    return std::move(oss).str();
}

std::string toUpperLatin1(const std::string_view str)
{
    std::ostringstream oss;
    for (char c : str) {
        oss << toUpperLatin1(c);
    }
    return std::move(oss).str();
}

std::string toLowerLatin1(std::string str)
{
    if (isLowerLatin1(str)) {
        return str; // note: move would be a pessimization
    }

    return toLowerLatin1(std::string_view{str});
}

std::string toUpperLatin1(std::string str)
{
    if (isUpperLatin1(str)) {
        return str; // note: move would be a pessimization
    }

    return toUpperLatin1(std::string_view{str});
}
