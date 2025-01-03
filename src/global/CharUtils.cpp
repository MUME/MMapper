// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "CharUtils.h"

#include "Consts.h"
#include "tests.h"

#include <deque>
#include <iomanip>
#include <iostream>
#include <vector>

namespace { // anonymous

void test_foreachCharSingle()
{
    using namespace char_consts;
    using namespace string_consts;

    auto testcase = [](std::string_view input, std::vector<std::string_view> expect) {
        size_t arg = 0;
        foreachAsciiCharSingle(
            input,
            C_SEMICOLON,
            [&expect, &arg]() {
                TEST_ASSERT(arg < expect.size());
                TEST_ASSERT(expect.at(arg) == SV_SEMICOLON);
                ++arg;
            },
            [&expect, &arg](std::string_view s) {
                TEST_ASSERT(arg < expect.size());
                TEST_ASSERT(s.find(C_SEMICOLON) == std::string_view::npos);
                TEST_ASSERT(expect.at(arg) == s);
                ++arg;
            });
        TEST_ASSERT(arg == expect.size());
    };

    testcase("", {""});
    testcase("a", {"a"});
    testcase("ab", {"ab"});
    testcase(";", {"", ";", ""});
    testcase(";;", {"", ";", "", ";", ""});
    testcase("a;", {"a", ";", ""});
    testcase("a;;", {"a", ";", "", ";", ""});
    testcase(";a", {"", ";", "a"});
    testcase(";;a", {"", ";", "", ";", "a"});
    testcase(";;a;", {"", ";", "", ";", "a", ";", ""});
    testcase("ab;;c;", {"ab", ";", "", ";", "c", ";", ""});
}

void test_foreachCharMulti2()
{
    auto testcase = [](std::string_view input, char c, std::vector<std::string_view> expect) {
        size_t arg = 0;
        foreachCharMulti2(input, c, [&expect, &arg](std::string_view s) {
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
} // namespace

namespace test {
void testCharUtils()
{
    test_foreachCharSingle();
    test_foreachCharMulti2();
}
} // namespace test
