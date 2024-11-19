// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "AnsiOstream.h"

#include "AnsiTextUtils.h"
#include "CharUtils.h"
#include "Charset.h"
#include "LineUtils.h"
#include "PrintUtils.h"
#include "tests.h"

#include <sstream>

void AnsiOstream::writeQuotedWithColor(const RawAnsi &normalAnsi,
                                       const RawAnsi &escapeAnsi,
                                       const std::string_view sv,
                                       const bool includeQuotes)
{
    using namespace token_stream;
    auto raii = getStateRestorer();
    setNextAnsi(normalAnsi);

    auto writeHelper = [this, escapeAnsi](const CharTokenTypeEnum type, const auto x) {
        switch (type) {
        case CharTokenTypeEnum::Normal:
            return write(x);
        case CharTokenTypeEnum::Escaped:
            return writeWithColor(escapeAnsi, x);
        }
        std::abort();
    };

    CallbackCharTokenStream ts{[&writeHelper](const CharTokenTypeEnum type, const char c) {
                                   writeHelper(type, c);
                               },
                               [&writeHelper](const CharTokenTypeEnum type, const char32_t c) {
                                   writeHelper(type, c);
                               },
                               [&writeHelper](const CharTokenTypeEnum type,
                                              const std::string_view bytes) {
                                   if (!bytes.empty()) {
                                       writeHelper(type, bytes);
                                   }
                               }};
    print_string_quoted(ts, sv, includeQuotes);
}

void AnsiOstream::transition(const RawAnsi to)
{
    if (m_currentAnsi == to) {
        return;
    }

    ansi_transition(getOstream(), getSupport(), m_currentAnsi, to);
    m_currentAnsi = to;
}

void AnsiOstream::writeLowLevel(const std::string_view sv)
{
    if (sv.empty()) {
        return;
    }

    assert(sv.find(char_consts::C_NEWLINE) == sv.npos);
    assert(sv.find(char_consts::C_ESC) == sv.npos);

    if (m_nextAnsi) {
        transition(*m_nextAnsi);
        m_nextAnsi.reset();
    }

    m_hasNewline = false;
    getOstream() << sv;
}

void AnsiOstream::writeNewline()
{
    // We want to preserve the next ANSI across the newline,
    // so we need to know what would be written next (might be current).
    m_nextAnsi = getNextAnsi();
    transition(RawAnsi{});
    m_hasNewline = true;
    getOstream() << char_consts::C_NEWLINE;
}

void AnsiOstream::close()
{
    m_nextAnsi.reset();
    transition(RawAnsi{});
}

void AnsiOstream::write(const char c)
{
    assert(charset::isAscii(c));
    write(std::string_view(&c, 1));
}

void AnsiOstream::write(const char16_t codepoint)
{
    assert(!charset::utf16_detail::is_utf16_surrogate(codepoint));
    write(static_cast<char32_t>(codepoint));
}

void AnsiOstream::write(const char32_t codepoint)
{
    write(charset::conversion::utf32toUtf8(codepoint));
}

void AnsiOstream::write(const QuotedString &str)
{
    writeQuoted(str.m_str);
}

void AnsiOstream::write(const SmartQuotedString &str)
{
    writeSmartQuoted(str.m_str);
}

void AnsiOstream::writeQuoted(std::string_view sv)
{
    const auto next = getNextAnsi();
    writeQuotedWithColor(next, next.withToggledReverse(), sv);
}

void AnsiOstream::writeSmartQuoted(std::string_view sv)
{
    if (!requiresQuote(sv)) {
        write(sv);
        return;
    }

    writeQuoted(sv);
}

void AnsiOstream::write(const std::string_view sv)
{
    foreachLine(sv, [this](std::string_view line) {
        if (line.empty()) {
            return;
        }
        const bool hasNewline = line.back() == char_consts::C_NEWLINE;
        trim_newline_inplace(line);

        if (!line.empty()) {
            // The character we're replacing is ASCII, while the rest of the string is allowed to
            // be any of ASCII, Latin-1, or Utf-8.
            foreachAsciiCharSingle(
                line,
                char_consts::C_ESC,
                [this]() {
                    const std::string_view esc_replacement = "<ESC>";
                    writeLowLevel(esc_replacement);
                },
                [this](const std::string_view withoutEscape) {
                    if (!withoutEscape.empty()) {
                        writeLowLevel(withoutEscape);
                    }
                });
        }

        if (hasNewline) {
            writeNewline();
        }
    });
}

void AnsiOstream::writeWithEmbeddedAnsi(const std::string_view sv)
{
    using char_consts::C_ESC;
    foreachAnsi(
        sv,
        [this](std::string_view ansiColorString) {
            assert(!ansiColorString.empty());
            assert(ansiColorString.front() == C_ESC);
            assert(isAnsiColor(ansiColorString));
            if (const auto optAnsi = ansi_parse(getNextAnsi(), ansiColorString)) {
                setNextAnsi(optAnsi.value());
            } else {
                // REVISIT: display or filter it the cases that fail to parse?
                // This will replace the ESC with "<ESC>"
                write(ansiColorString);
            }
        },
        [this](std::string_view invalidAnsi) {
            assert(!invalidAnsi.empty());
            assert(invalidAnsi.front() == C_ESC);
            // This will replace the ESC with "<ESC>"
            write(invalidAnsi);
        },
        [this](std::string_view nonAnsi) { write(nonAnsi); });
}

namespace { // anonymous
void test_aos1()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        aos.writeWithColor(getRawAnsi(AnsiColor16Enum::green).withBold(), "Test");
        TEST_ASSERT(oss.str() == "\033[1;32mTest");
        TEST_ASSERT(aos.getNextAnsi() == RawAnsi{});
    }
    TEST_ASSERT(oss.str() == "\033[1;32mTest\033[0m");
}
void test_aos2()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        auto color = getRawAnsi(AnsiColor16Enum::blue).withBold();
        aos << color;
        aos.writeWithEmbeddedAnsi("\033[21mNotBold\033\033x\033[1n");
        TEST_ASSERT(aos.getNextAnsi() == getRawAnsi(AnsiColor16Enum::blue));
    }
    TEST_ASSERT(oss.str() == "\033[34mNotBold<ESC><ESC>x<ESC>[1n\033[0m");
}
void test_aos3a()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        auto color = getRawAnsi(AnsiColor16Enum::blue);
        aos << color;
        color.setUnderline();
        aos.writeWithEmbeddedAnsi("\033[4:1mx");
        TEST_ASSERT(aos.getNextAnsi() == color);
    }
    TEST_ASSERT(oss.str() == "\033[4;34mx\033[0m");
}
void test_aos3b()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        auto color = getRawAnsi(AnsiColor16Enum::blue);
        aos << color.withUnderline();
        // "4:" is treated like "4:0", which is the same as "24" (remove underline).
        aos.writeWithEmbeddedAnsi("\033[4:mx");
        TEST_ASSERT(aos.getNextAnsi() == color);
    }
    TEST_ASSERT(oss.str() == "\033[34mx\033[0m");
}
void test_aos4()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        auto color = getRawAnsi(AnsiColor16Enum::blue);
        color.setUnderline();
        aos << color;
        aos.writeWithEmbeddedAnsi("\033[7:5:3mx"); // nonsense
        TEST_ASSERT(aos.getNextAnsi() == color);
    }
    TEST_ASSERT(oss.str() == "\033[4;34mx\033[0m");
}
void test_aos5a()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        RawAnsi color;
        color.setUnderline();
        aos << color.withBold();
        aos.writeWithEmbeddedAnsi("\033[21;4mx");
        TEST_ASSERT(aos.getNextAnsi() == color);
    }
    TEST_ASSERT(oss.str() == "\033[4mx\033[0m");
}
void test_aos5b()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        RawAnsi color;
        color.setUnderline();
        aos << color.withBold();
        aos << "x";
        aos.writeWithEmbeddedAnsi("\033[21;4my");
        TEST_ASSERT(aos.getNextAnsi() == color);
    }
    TEST_ASSERT(oss.str() == "\033[1;4mx\033[0;4my\033[0m");
}
void test_aos5c()
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        RawAnsi color;
        color.setItalic();
        aos << color.withBold().withUnderline();
        aos << "x";
        aos.writeWithEmbeddedAnsi("\033[21;24my");
        TEST_ASSERT(aos.getNextAnsi() == color);
    }
    TEST_ASSERT(oss.str() == "\033[1;3;4mx\033[0;3my\033[0m");
}
void test_aos6()
{
    // demonstrate state restorer using writes containing newlines
    std::ostringstream oss;
    {
        auto a = getRawAnsi(AnsiColor16Enum::red, AnsiColor16Enum::blue)
                     .withBold()
                     .withItalic()
                     .withStrikeout()
                     .withUnderline();
        AnsiOstream aos{oss};
        aos.setNextAnsi(a);
        {
            const auto b = a.withForeground(AnsiColor16Enum::green).withoutUnderline().withBlink();

            // this block is equivalent to writeWithColor(), except we won't
            // assume that function is implemented with the state restorer.
            auto raii = aos.getStateRestorer();
            aos.setNextAnsi(b);
            aos.write("ABC\nDEF"); // note the lack of newline here.
        }
        aos.write("GHI\nJKL");
    }

    // observe the transition on the 2nd line adds back red + underline, and removes blink.
    TEST_ASSERT(oss.str()
                == "\033[1;3;5;9;32;44mABC\033[0m\n"
                   "\033[1;3;5;9;32;44mDEF\033[4;25;31mGHI\033[0m\n"
                   "\033[1;3;9;4;31;44mJKL\033[0m");
}
} // namespace

void test::testAnsiOstream()
{
    test_aos1();
    test_aos2();
    test_aos3a();
    test_aos3b();
    test_aos4();
    test_aos5a();
    test_aos5b();
    test_aos5c();
    test_aos6();
}
