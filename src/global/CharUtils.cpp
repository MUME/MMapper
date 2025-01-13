// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "CharUtils.h"

#include "Consts.h"
#include "tests.h"

#include <deque>
#include <iomanip>
#include <iostream>
#include <vector>

namespace test {
void testCharUtils()
{
    auto testcase = [](std::string_view input, char c, std::vector<std::string_view> expect) {
        size_t arg = 0;
        foreachChar(input, c, [&expect, &arg](std::string_view s) {
            TEST_ASSERT(arg < expect.size());
            TEST_ASSERT(expect.at(arg) == s);
            ++arg;
        });
        TEST_ASSERT(arg == expect.size());
    };

    using char_consts::C_SEMICOLON;
    testcase("", C_SEMICOLON, {});
    testcase("a", C_SEMICOLON, {"a"});
    testcase("ab", C_SEMICOLON, {"ab"});
    testcase(";", C_SEMICOLON, {";"});
    testcase(";;", C_SEMICOLON, {";;"});
    testcase("a;", C_SEMICOLON, {"a", ";"});
    testcase("a;;", C_SEMICOLON, {"a", ";;"});
    testcase(";a", C_SEMICOLON, {";", "a"});
    testcase(";;a", C_SEMICOLON, {";;", "a"});
    testcase(";;a;", C_SEMICOLON, {";;", "a", ";"});
    testcase("ab;;c;", C_SEMICOLON, {"ab", ";;", "c", ";"});
}
} // namespace test
