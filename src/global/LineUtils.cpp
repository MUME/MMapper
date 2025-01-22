// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "LineUtils.h"

#include "Charset.h"
#include "TextUtils.h"
#include "tests.h"

namespace mmqt {

int countLines(const QStringView input)
{
    int count = 0;
    foreachLine(input, [&count](const QStringView, bool) { ++count; });
    return count;
}

int countLines(const QString &input)
{
    return countLines(QStringView{input});
}

} // namespace mmqt

size_t countLines(const std::string_view input)
{
    size_t count = 0;
    foreachLine(input, [&count](const std::string_view) { ++count; });
    return count;
}

namespace mmqt {
NODISCARD QString getCommand(const QByteArray &utf8_line)
{
    using namespace char_consts;
    // note: it's tempting to think we could just convert to latin1 or ascii codepoints here,
    // but the decision about whether or not to transliterate the input  needs to be made by
    // the command itself, since commands for editing room notes may want to include utf8,
    // and smart quotes probably shouldn't be treated the same as quote characters
    // for command arguments that require single or double quotes.

    std::u32string buffer;
    auto sv = mmqt::toStdStringViewUtf8(utf8_line);
    charset::foreach_codepoint_utf8(sv, [&buffer](const char32_t codepoint) {
        switch (codepoint) {
        case C_BACKSPACE:
            if (!buffer.empty()) {
                buffer.resize(buffer.size() - 1);
            }
            break;
        case C_CTRL_U: {
            // removing after newline: s/[^\n]$//
            auto newline_pos = buffer.find_last_of(C_NEWLINE);
            if (newline_pos == std::u32string::npos) {
                buffer.clear();
            } else {
                buffer.resize(newline_pos + 1);
            }
            break;
        }
        case C_CTRL_W: {
            // remove spaces, and then remove a word: s/[^ \t\r\n]+[ \t]*$//
            const auto non_space_it = std::find_if_not(buffer.rbegin(),
                                                       buffer.rend(),
                                                       [](const char32_t c) -> bool {
                                                           return c == C_SPACE || c == C_TAB;
                                                       });

            const auto wordstart_it = std::find_if(non_space_it,
                                                   buffer.rend(),
                                                   [](const char32_t c) -> bool {
                                                       switch (c) {
                                                       case C_TAB:
                                                       case C_SPACE:
                                                       case C_NEWLINE:
                                                       case C_CARRIAGE_RETURN:
                                                           return true;
                                                       default:
                                                           return false;
                                                       }
                                                   });

            if (wordstart_it == buffer.rend()) {
                buffer.clear();
            } else {
                const auto to_remove = wordstart_it - buffer.rbegin();
                static_assert(std::is_signed_v<decltype(to_remove)>); // reason for static cast
                buffer.resize(buffer.size() - static_cast<size_t>(to_remove));
            }
            break;
        }
        default:
            buffer += codepoint;
            break;
        }
    });

    // it's tempting to use .simplified() here, but again it's up to the command to decide
    // whether extra whitespace (including tabs) is important or not, because commands can
    // require "quoted strings" and some commands affect user data such as room notes.
    return QString::fromStdU32String(buffer).trimmed();
}
} // namespace mmqt

namespace test {
void testLineUtils()
{
    using mmqt::getCommand;
    using namespace char_consts;

    const char ARR_zapLine[2]{C_CTRL_U, C_NUL};
    const char ARR_removeWord[2]{C_CTRL_W, C_NUL};
    const QByteArray zapLine = ARR_zapLine;
    const QByteArray removeWord = ARR_removeWord;

    TEST_ASSERT(getCommand(" ") == "");
    TEST_ASSERT(getCommand(" x ") == "x");
    TEST_ASSERT(getCommand("\nx\n") == "x");

    TEST_ASSERT(getCommand("\bx") == "x");
    TEST_ASSERT(getCommand("x\by") == "y");
    TEST_ASSERT(getCommand("xx\by") == "xy");
    TEST_ASSERT(getCommand("xx" + removeWord + "y") == "y");       // ctrl+W
    TEST_ASSERT(getCommand("x\nyy" + removeWord + "z") == "x\nz"); // ctrl+W

    TEST_ASSERT(getCommand("x\ny y" + removeWord + "z") == "x\ny z"); // ctrl+W
    TEST_ASSERT(getCommand("x\ny y" + zapLine + "z") == "x\nz");      // ctrl+U

    TEST_ASSERT(getCommand("x x " + removeWord + "y") == "x y"); // ctrl+W
    TEST_ASSERT(getCommand("x x " + zapLine + "y") == "y");      // ctrl+U
}
} // namespace test
