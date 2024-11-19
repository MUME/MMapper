// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "sanitizer.h"

#include "../global/AnsiTextUtils.h"
#include "../global/Charset.h"
#include "../global/ConfigConsts.h"
#include "../global/LineUtils.h"
#include "../global/PrintUtils.h"
#include "../global/parserutils.h"
#include "../global/tests.h"

#include <optional>
#include <ostream>
#include <sstream>
#include <tuple>

using char_consts::C_ESC;
using char_consts::C_NBSP;
using char_consts::C_NEWLINE;
using char_consts::C_SPACE;
using namespace string_consts;

namespace sanitizer {

namespace detail {

namespace nbsp_aware {

NODISCARD static bool isAnySpace(const char c)
{
    return ascii::isSpace(c) || c == C_NBSP;
}

static void trimLeft(std::string_view &sv)
{
    while (!sv.empty() && isAnySpace(sv.front())) {
        sv.remove_prefix(1);
    }
}
static void trimRight(std::string_view &sv)
{
    while (!sv.empty() && isAnySpace(sv.back())) {
        sv.remove_suffix(1);
    }
}
NODISCARD static std::string_view takeWord(std::string_view &sv)
{
    std::string_view result = sv;
    while (!sv.empty() && !isAnySpace(sv.front())) {
        sv.remove_prefix(1);
    }
    if (!sv.empty()) {
        result.remove_suffix(sv.length());
    }
    assert(!result.empty());
    return result;
}
} // namespace nbsp_aware

NODISCARD static bool hasBogusWhitespaceOneline(const std::string_view line)
{
    assert(!line.empty()); // caller needs to check this.

    if (ascii::isSpace(line.front()) || ascii::isSpace(line.back())) {
        return true;
    }

    bool wasSpace = false;

    for (const char c : line) {
        if (c == C_NEWLINE) {
            assert(false);
            return true;
        }
        const bool isSpace = nbsp_aware::isAnySpace(c);
        if (isSpace && wasSpace) {
            return true;
        }
        wasSpace = isSpace;
    }

    return false;
}

NODISCARD static bool hasBogusWhitespace(const std::string_view desc)
{
    if (desc.empty()) {
        return false;
    }

    if (desc.back() != C_NEWLINE) {
        return true;
    }

    if (desc.front() == C_NEWLINE) {
        return true;
    }

    if (desc.size() >= 2 && desc.back() == C_NEWLINE && *std::next(desc.rbegin()) == C_NEWLINE) {
        return true;
    }

    // Note: foreachLine doesn't have an early-bail-out / short-circuit option,
    // so we'll just rely on the fact that most descs are short.
    bool bogus = false;
    foreachLine(desc, [&bogus](std::string_view line) {
        if (line.empty()) {
            return;
        }
        trim_newline_inplace(line);
        if (line.empty()) {
            return;
        }
        if (hasBogusWhitespaceOneline(line)) {
            bogus = true;
        }
    });
    return bogus;
}

static void test_bogus_whitespace()
{
    TEST_ASSERT(!hasBogusWhitespaceOneline("a"));
    TEST_ASSERT(hasBogusWhitespaceOneline("a\n"));

    // empty isn't bogus
    TEST_ASSERT(!hasBogusWhitespace(""));

    // lacking a trailing newline is bogus
    TEST_ASSERT(hasBogusWhitespace("a"));
    TEST_ASSERT(hasBogusWhitespace("a "));
    TEST_ASSERT(hasBogusWhitespace(" a"));
    TEST_ASSERT(hasBogusWhitespace("a b"));
    TEST_ASSERT(hasBogusWhitespace("a  b"));

    // newlines
    TEST_ASSERT(!hasBogusWhitespace("a\n"));
    TEST_ASSERT(!hasBogusWhitespace("a\nb\n"));
    TEST_ASSERT(hasBogusWhitespace("\n\n"));
    TEST_ASSERT(hasBogusWhitespace("a\n\n"));
    TEST_ASSERT(hasBogusWhitespace("\na\n"));

    // blank lines are allowed!
    TEST_ASSERT(!hasBogusWhitespace("a\n\nb\n"));
}

NODISCARD static bool isPureAsciiChar(const char c)
{
    return c == C_NEWLINE || (isAscii(c) && isPrintLatin1(c));
}

NODISCARD static bool isPureAscii(const std::string_view sv)
{
    return std::all_of(sv.begin(), sv.end(), isPureAsciiChar);
}

NODISCARD static bool needsSanitizer_multiline(const std::string_view sv)
{
    return hasBogusWhitespace(sv) || !isPureAscii(sv);
}

NODISCARD static bool needsSanitizer_wordwrapped(const std::string_view sv, size_t width)
{
    if (needsSanitizer_multiline(sv)) {
        return true;
    }

    bool needs_fix = false;
    std::optional<std::string_view> last;
    foreachLine(sv, [width, &needs_fix, &last](std::string_view line) {
        if (needs_fix || line.empty()) {
            return;
        }

        trim_newline_inplace(line);
        if (line.empty()) {
            return;
        }

        if (nbsp_aware::isAnySpace(line.front()) || nbsp_aware::isAnySpace(line.back())) {
            needs_fix = true;
            return;
        }

        // verify that it's wrapped to the limit
        if (line.size() > width) {
            // allowed exception: first word can be to long
            if (line.find(C_SPACE) != std::string_view::npos) {
                needs_fix = true;
            }
            return;
        }

        // verify that it's packed as tightly as possible
        if (last) {
            auto tmp = line;
            const auto word = nbsp_aware::takeWord(tmp);
            if (last->size() + 1 + word.size() <= width) {
                needs_fix = true;
                return;
            }
        }
        last = line;
    });
    return needs_fix;
}

static void addSanitized(std::ostream &os, const std::string_view sv)
{
    if (isPureAscii(sv)) {
        os << sv;
        return;
    }

    charset::conversion::utf8ToAscii(os, sv);
}

// returns true if it inserted anything
NODISCARD static bool sanitize_oneline(std::ostream &os, std::string_view line)
{
    nbsp_aware::trimLeft(line);
    nbsp_aware::trimRight(line);

    bool first = true;
    while (!line.empty()) {
        const auto word = nbsp_aware::takeWord(line);
        if (word.empty()) {
            std::abort();
        }

        if (first) {
            first = false;
        } else {
            os << C_SPACE;
        }

        addSanitized(os, word);
        nbsp_aware::trimLeft(line);
    }

    return !first;
}

NODISCARD static bool sanitize_wordwrapped(std::ostream &os, std::string_view input, size_t width)
{
    nbsp_aware::trimLeft(input);
    nbsp_aware::trimRight(input);

    bool inserted = false;
    size_t current_line_length = 0;
    while (!input.empty()) {
        const auto word = nbsp_aware::takeWord(input);
        if (word.empty()) {
            std::abort();
        }

        if (current_line_length != 0) {
            if (current_line_length + 1 + word.size() > width) {
                os << C_NEWLINE;
                current_line_length = 0;
            } else {
                os << C_SPACE;
                current_line_length += 1;
            }
        }
        inserted = true;
        addSanitized(os, word);
        current_line_length += word.size();
        nbsp_aware::trimLeft(input);
    }
    if (current_line_length != 0) {
        os << C_NEWLINE;
    }
    return inserted;
}

NODISCARD static SanitizedString sanitize_wordwrapped(std::string input, const size_t width)
{
    if (!needsSanitizer_wordwrapped(input, width)) {
        return SanitizedString{std::move(input)};
    }

    if (containsAnsi(input)) {
        input = strip_ansi(std::move(input));
    }

    if (!isPureAscii(input)) {
        input = charset::conversion::utf8ToAscii(input);
    }

    std::string_view sv = input;
    nbsp_aware::trimLeft(sv);
    nbsp_aware::trimRight(sv);

    std::ostringstream os;
    std::ignore = sanitize_wordwrapped(os, sv, width);

    auto str = std::move(os).str();
    assert(!needsSanitizer_wordwrapped(str, width));
    return SanitizedString{std::move(str)};
}

NODISCARD static SanitizedString sanitize_multiline(std::string input)
{
    if (!needsSanitizer_multiline(input)) {
        return SanitizedString{std::move(input)};
    }

    if (containsAnsi(input)) {
        input = strip_ansi(std::move(input));
    }

    if (!isPureAscii(input)) {
        input = charset::conversion::utf8ToAscii(input);
    }

    // Note: This function has to use NBSP-aware functions.
    std::string_view sv = input;
    nbsp_aware::trimLeft(sv);
    nbsp_aware::trimRight(sv);

    std::ostringstream os;
    foreachLine(sv, [&os](const std::string_view line) {
        if (sanitize_oneline(os, line)) {
            os << C_NEWLINE;
        }
    });

    auto str = std::move(os).str();
    assert(!needsSanitizer_multiline(str));
    return SanitizedString{std::move(str)};
}

static void test_sanitize_wordwrap()
{
    const auto input
        = "This small height once was a place of death. The D\u00FAnedain never penalized\n"
          "anyone with death, but orcs hung people in large numbers - a far more\n"
          "merciful fate than the one that could await you at their torturers in the\n"
          "dungeons of the old, ruined castle that towers south of here.\n";

    const auto expect
        = "This small height once was a place of death. The Dunedain never penalized anyone\n"
          "with death, but orcs hung people in large numbers - a far more merciful fate\n"
          "than the one that could await you at their torturers in the dungeons of the old,\n"
          "ruined castle that towers south of here.\n";

    const auto output = sanitize_wordwrapped(input, 80);
    TEST_ASSERT(output.getStdStringViewUtf8() == expect);
}

static void test_sanitize_multiline()
{
    auto testcase = [](const std::string_view input, const std::string_view expect) {
        const SanitizedString output = sanitize_multiline(std::string{input});
        const std::string_view got = output.getStdStringUtf8();
        TEST_ASSERT(got == expect);
    };

    TEST_ASSERT(!isPureAscii(SV_ESC));
    TEST_ASSERT(!isPureAscii(SV_NBSP));

    testcase("", "");
    testcase("\n", "");
    testcase("\n\n", "");
    testcase("\n \n", "");
    testcase("\n \n \n", "");

    testcase("a ", "a\n");
    testcase(" a", "a\n");
    testcase(" a", "a\n");
    testcase("a b", "a b\n");
    testcase("a  b", "a b\n");

    testcase("a\n", "a\n");
    testcase("\na\n", "a\n");
    testcase("\na \n", "a\n");
    testcase("\n a\n", "a\n");
    testcase("\na b\n", "a b\n");
    testcase("\na  b\n", "a b\n");

    // nbsp still counts as whitespace, so the same newline normalization rules apply
    testcase("\u00A0", ""); // NBSP
    testcase("a \u00A0 b", "a b\n");

    // leading and trailing newline normalization
    testcase("\na", "a\n");
    testcase("\na\n", "a\n");
    testcase("a\n\n", "a\n");

    testcase("\033[31ma\033[0m", "a\n");
}
} // namespace detail

bool isSanitizedOneLine(const std::string_view input)
{
    if (input.empty()) {
        return true;
    }
    // REVISIT: This doesn't actually test if it's just one line.
    return !detail::hasBogusWhitespaceOneline(input) && detail::isPureAscii(input);
}

// REVISIT: This could probably just take a string_view now that it no longer returns a std::string
SanitizedString sanitizeOneLine(std::string input)
{
    if (isSanitizedOneLine(input)) {
        return SanitizedString{std::move(input)};
    }

    if (::containsAnsi(input)) {
        input = ::strip_ansi(input);
    }

    if (!detail::isPureAscii(input)) {
        input = charset::conversion::utf8ToAscii(input);
    }

    // force it to be one line
    for (char &c : input) {
        if (detail::nbsp_aware::isAnySpace(c)) {
            c = C_SPACE;
        }
    }

    std::ostringstream os;
    std::ignore = detail::sanitize_oneline(os, input);
    auto output = std::move(os).str();

    assert(isSanitizedOneLine(output));
    return SanitizedString{std::move(output)};
}

bool isSanitizedWordWraped(const std::string_view desc, const size_t width)
{
    return !detail::needsSanitizer_wordwrapped(desc, width);
}

SanitizedString sanitizeWordWrapped(std::string desc, const size_t width)
{
    return detail::sanitize_wordwrapped(std::move(desc), width);
}

bool isSanitizedMultiline(const std::string_view desc)
{
    return !detail::needsSanitizer_multiline(desc);
}

// REVISIT: This could probably just take a string_view now that it no longer returns a std::string
SanitizedString sanitizeMultiline(std::string desc)
{
    return detail::sanitize_multiline(std::move(desc));
}

namespace detail::user_supplied {
NODISCARD static bool isSanitized(const std::string_view input)
{
    if (input.empty()) {
        return true;
    }

    // allows leading whitespace, but not trailing whitespace.
    // also must end in "\n" (not "\r\n").
    static auto validLine = [](std::string_view line) -> bool {
        assert(!line.empty());
        if (line.back() != C_NEWLINE) {
            return false;
        }
        line.remove_suffix(1);
        return line.empty() || !ascii::isSpace(line.back());
    };

    bool result = true;
    foreachLine(input, [&result](std::string_view line) {
        assert(!line.empty());
        if (result && !validLine(line)) {
            result = false;
        }
    });
    return result;
}

static void sanitize(std::string &input)
{
    if (isSanitized(input)) {
        return;
    }

    std::ostringstream os;
    foreachLine(input, [&os](std::string_view line) {
        // remove all trailing whitespace
        while (!line.empty()) {
            const char c = line.back();
            if (c != C_NBSP && !ascii::isSpace(c)) {
                break;
            }
            line.remove_suffix(1);
        }

        if (!line.empty()) {
            os << line;
        }
        os << SV_NEWLINE;
    });

    auto tmp = std::move(os).str();
    assert(isSanitized(tmp));
    std::swap(input, tmp);
}

} // namespace detail::user_supplied

bool isSanitizedUserSupplied(const std::string_view desc)
{
    return detail::user_supplied::isSanitized(desc);
}

SanitizedString sanitizeUserSupplied(std::string desc)
{
    if (isSanitizedUserSupplied(desc)) {
        return SanitizedString{std::move(desc)};
    }

    detail::user_supplied::sanitize(desc);
    return SanitizedString{std::move(desc)};
}

} // namespace sanitizer

namespace { // anonymous
void test_conversion_to_ascii()
{
    const std::string utf8 = "D\u00FAnedain";
    const std::string utf8_with_newline = utf8 + S_NEWLINE;
    const std::string ascii = "Dunedain";
    const std::string ascii_with_newline = ascii + S_NEWLINE;

    TEST_ASSERT(charset::isValidUtf8(utf8));
    TEST_ASSERT(charset::isValidUtf8(ascii));
    TEST_ASSERT(!sanitizer::detail::isPureAscii(utf8));
    TEST_ASSERT(sanitizer::sanitizeOneLine(utf8).getStdStringViewUtf8() == ascii);
    TEST_ASSERT(sanitizer::sanitizeMultiline(utf8).getStdStringViewUtf8() == ascii_with_newline);
    TEST_ASSERT(sanitizer::sanitizeWordWrapped(utf8, 80).getStdStringViewUtf8()
                == ascii_with_newline);

    // User supplied (e.g. room note) is allowed to contain non-ascii characters.
    TEST_ASSERT(sanitizer::sanitizeUserSupplied(utf8).getStdStringViewUtf8() == utf8_with_newline);
}

} // namespace

namespace test {
void testSanitizer()
{
    ::sanitizer::detail::test_bogus_whitespace();
    ::sanitizer::detail::test_sanitize_multiline();
    ::sanitizer::detail::test_sanitize_wordwrap();
    test_conversion_to_ascii();
}
} // namespace test
