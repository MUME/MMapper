// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "CharUtils.h"

#include "Consts.h"

#include <deque>
#include <iomanip>
#include <iostream>
#include <vector>

namespace { // anonymous
namespace test {

NODISCARD static int testing()
{
    auto test = [](std::string_view input, char c, std::vector<std::string_view> expect) {
        size_t arg = 0;
        foreachChar(input, c, [&expect, &arg](std::string_view s) {
            assert(arg < expect.size());
            assert(expect.at(arg) == s);
            ++arg;
        });
        assert(arg == expect.size());
    };

    using char_consts::C_SEMICOLON;
    test("", C_SEMICOLON, {});
    test("a", C_SEMICOLON, {"a"});
    test("ab", C_SEMICOLON, {"ab"});
    test(";", C_SEMICOLON, {";"});
    test(";;", C_SEMICOLON, {";;"});
    test("a;", C_SEMICOLON, {"a", ";"});
    test("a;;", C_SEMICOLON, {"a", ";;"});
    test(";a", C_SEMICOLON, {";", "a"});
    test(";;a", C_SEMICOLON, {";;", "a"});
    test(";;a;", C_SEMICOLON, {";;", "a", ";"});
    test("ab;;c;", C_SEMICOLON, {"ab", ";;", "c", ";"});

    return 42;
}

static int test = testing();
} // namespace test
} // namespace
