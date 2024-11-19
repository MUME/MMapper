// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/AnsiOstream.h"
#include "../global/logging.h"
#include "../global/tests.h"
#include "Diff.h"

#include <sstream>

namespace { // anonymous
void test_diff(const std::string_view a, const std::string_view b, const std::string_view expect)
{
    std::ostringstream os;
    os << "Testing ";
    print_string_quoted(os, a);
    os << " vs ";
    print_string_quoted(os, b);
    os << " ...\n";

    std::ostringstream resultOs;
    {
        AnsiOstream aos{resultOs};
        printDiff(aos, a, b);
    }
    auto resultStr = std::move(resultOs).str();
    os << "Yields:\n" << resultStr << "\n" << std::flush;
    MMLOG() << std::move(os).str();
    TEST_ASSERT(resultStr == expect);
}
} // namespace

namespace test {
void testMapDiff()
{
    // NOTE: We can't test strings that are exactly equal, because the diff contains an assert
    // that they're different. Should we change it to allow testing equal strings?
    ::test_diff(
        "a\nb\nc",
        "a\nB\nc",
        "@ \033[33m\"\033[0ma\033[93m\\n\033[33m\"\033[0m\n@ \033[33m\"\033[31mb\033[0m \033[32mB\033[93m\\n\033[33m\"\033[0m\n@ \033[33m\"\033[0mc\033[33m\"\033[0m\n");

    // adjacent trailing dots are grouped together as a single token,
    // but dots aren't tokenized in the middle of a word.
    ::test_diff("a.", "a..", "@ \033[33m\"\033[0ma \033[31m.\033[0m \033[32m..\033[33m\"\033[0m\n");
    ::test_diff("a.b", "a..b", "@ \033[33m\"\033[31ma.b\033[0m \033[32ma..b\033[33m\"\033[0m\n");

    // This current test reflects the fact that punctation is not cuddled;
    // the expected output will need to change when you fix the diff printer.
    ::test_diff(
        "a, $b c.",
        "A, b c!",
        "@ \033[33m\"\033[31ma\033[0m \033[32mA\033[0m , \033[31m$\033[0m b c \033[31m.\033[0m \033[32m!\033[33m\"\033[0m\n");
}
} // namespace test
