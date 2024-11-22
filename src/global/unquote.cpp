// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "unquote.h"

#include "Consts.h"
#include "TextUtils.h"
#include "logging.h"
#include "tests.h"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

using ascii::isSpace;
using namespace char_consts;

enum class NODISCARD ReasonEnum {
    INVALID_ESCAPE,
    INVALID_OCTAL,
    INVALID_HEX,
    UNBALANCED_QUOTES,
};

struct NODISCARD UnquoteException final : public std::runtime_error
{
    const ReasonEnum reason;
    explicit UnquoteException(const ReasonEnum reason_) noexcept(
        noexcept(std::runtime_error("UnquoteException")))
        : std::runtime_error("UnquoteException")
        , reason{reason_}
    {}
    ~UnquoteException() final;
};
UnquoteException::~UnquoteException() = default;

enum class NODISCARD TokenEnum { BeginString, EndString };

NODISCARD static std::optional<uint32_t> try_decode_oct(char c) noexcept
{
    if ('0' <= c && c <= '7') {
        return c - '0';
    } else {
        return std::nullopt;
    }
}

NODISCARD static std::optional<uint32_t> try_decode_hex(char c) noexcept
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return 10 + (c - 'a');
    } else if ('A' <= c && c <= 'F') {
        return 10 + (c - 'A');
    } else {
        return std::nullopt;
    }
}

NODISCARD static std::vector<std::string> unquote_unsafe(const std::string_view input,
                                                         const bool allowUnbalancedQuotes)
{
    using namespace char_consts;

    const auto foreach_char = [allowUnbalancedQuotes, &input](auto &&visit) -> void {
        const auto visit_oct = [&visit](auto &it, const auto end) -> void {
            uint32_t result = 0;
            for (int i = 0; i < 3; ++i) {
                if (it != end) {
                    if (auto opt = try_decode_oct(*it)) {
                        const auto bits = opt.value();
                        assert((bits & 0x7) == bits);
                        result <<= 3;
                        result |= bits & 0x7;
                        ++it;
                        continue;
                    }
                }
                throw UnquoteException(ReasonEnum::INVALID_OCTAL);
            }

            // 400..777 are invalid.
            if (result > 255) {
                throw UnquoteException(ReasonEnum::INVALID_OCTAL);
            }

            visit(static_cast<char>(static_cast<unsigned char>(result)));
        };

        const auto visit_hex = [&visit](const int digits, auto &it, const auto end) -> void {
            if (digits != 2 && digits != 4 && digits != 8) {
                throw std::runtime_error("internal error");
            }

            // NOTE: This is incorrect for parsing C++ escape codes:
            // * "\xF" is allowed in C++ but fails here.
            // * "\xFFF" is a parse error in C++, but it's allowed here.
            uint32_t result = 0;
            for (int i = 0; i < digits; ++i) {
                if (it != end) {
                    if (auto opt = try_decode_hex(*it)) {
                        const auto bits = opt.value();
                        assert((bits & 0xF) == bits);
                        result <<= 4;
                        result |= bits & 0xF;
                        ++it;
                        continue;
                    }
                }
                throw UnquoteException(ReasonEnum::INVALID_HEX);
            }

            static constexpr const auto DEFAULT_TRANSLIT = static_cast<uint32_t>(
                static_cast<unsigned char>(C_QUESTION_MARK));

            if (result > 255) {
                result = DEFAULT_TRANSLIT;
            }

            visit(static_cast<char>(static_cast<unsigned char>(result)));
        };

        enum class NODISCARD ModeEnum { Space, Other, DoubleQuote };

        ModeEnum mode = ModeEnum::Space;

        for (auto it = input.begin(), end = input.end(); it != end;) {
            const char c = *it++;

            if (mode == ModeEnum::Space) {
                if (isSpace(c)) {
                    continue;
                }

                visit(TokenEnum::BeginString);
                if (c == C_DQUOTE) {
                    mode = ModeEnum::DoubleQuote;
                } else {
                    mode = ModeEnum::Other;
                    visit(c);
                }
                continue;
            } else if (mode == ModeEnum::Other) {
                if (c == C_DQUOTE) {
                    mode = ModeEnum::DoubleQuote;
                } else if (isSpace(c)) {
                    mode = ModeEnum::Space;
                    visit(TokenEnum::EndString);
                } else {
                    visit(c);
                }
                continue;
            }

            assert(mode == ModeEnum::DoubleQuote);
            if (c == C_DQUOTE) {
                mode = ModeEnum::Other;
                continue;
            } else if (c != C_BACKSLASH) {
                visit(c);
                continue;
            }

            const char c2 = *it++;
            switch (c2) {
            case '0':
                visit(C_NUL);
                continue;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                throw UnquoteException(ReasonEnum::INVALID_ESCAPE);

            case 'a':
                visit(C_ALERT);
                continue;
            case 'b':
                visit(C_BACKSPACE);
                continue;
            case 'e': // Not valid C++
                visit(C_ESC);
                continue;
            case 'f':
                visit(C_FORM_FEED);
                continue;
            case 'n':
                visit(C_NEWLINE);
                continue;
            case 'r':
                visit(C_CARRIAGE_RETURN);
                continue;
            case 't':
                visit(C_TAB);
                continue;
            case 'v':
                visit(C_VERTICAL_TAB);
                continue;

            case C_SQUOTE:
            case C_DQUOTE:
            case C_QUESTION_MARK:
            case C_BACKSLASH:
                visit(c2);
                continue;

            case 'o':
                visit_oct(it, end);
                continue;

            case 'x':
                visit_hex(2, it, end);
                continue;
            case 'u':
                visit_hex(4, it, end);
                continue;
            case 'U':
                visit_hex(8, it, end);
                continue;

            default:
                throw UnquoteException(ReasonEnum::INVALID_ESCAPE);
            }
        }

        if (mode == ModeEnum::DoubleQuote) {
            if (allowUnbalancedQuotes) {
                visit(TokenEnum::EndString);
            } else {
                throw UnquoteException(ReasonEnum::UNBALANCED_QUOTES);
            }
        } else if (mode == ModeEnum::Other) {
            visit(TokenEnum::EndString);
        }
    };

    std::optional<size_t> current_length;
    std::vector<std::string> result;
    foreach_char([&current_length, &result](auto &&x) -> void {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, char>) {
            ++current_length.value();
        } else if constexpr (std::is_same_v<T, TokenEnum>) {
            switch (x) {
            case TokenEnum::BeginString:
                assert(!current_length.has_value());
                current_length.emplace(0);
                result.emplace_back();
                break;
            case TokenEnum::EndString:
                result.back().resize(current_length.value());
                current_length.reset();
                break;
            }
        } else {
            throw std::runtime_error("internal error");
        }
    });

    std::optional<size_t> current_pos;
    std::optional<size_t> stringNumber;

    foreach_char([&result, &stringNumber, &current_pos](auto &&x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, char>) {
            const size_t pos = current_pos.value()++;
            std::string &s = result[stringNumber.value()];
            assert(pos < s.length());
            s[pos] = static_cast<char>(x);
        } else if constexpr (std::is_same_v<T, TokenEnum>) {
            switch (x) {
            case TokenEnum::BeginString:
                assert(!current_pos.has_value());
                current_pos.emplace(0);
                if (!stringNumber.has_value()) {
                    stringNumber = 0;
                }
                break;
            case TokenEnum::EndString:
                assert(result[stringNumber.value()].size() == current_pos.value());
                current_pos.reset();
                ++stringNumber.value();
                break;
            }
        } else {
            throw std::runtime_error("internal error");
        }
    });

    assert(stringNumber.value_or(0) == result.size());
    return result;
}

NODISCARD UnquoteResult unquote(const std::string_view input,
                                const bool allowUnbalancedQuotes,
                                const bool allowEmbeddedNull)
{
    try {
        auto result = unquote_unsafe(input, allowUnbalancedQuotes);
        if (!allowEmbeddedNull) {
            for (std::string &s : result) {
                s = s.c_str(); // terminate every string at '\0'
            }
        }
        return UnquoteResult{std::move(result)};
    } catch (const UnquoteException &ex) {
        switch (ex.reason) {
        case ReasonEnum::INVALID_ESCAPE:
            return UnquoteResult{UnquoteFailureReason{"unquote: invalid escape"}};
        case ReasonEnum::INVALID_OCTAL:
            return UnquoteResult{UnquoteFailureReason{
                R"(unquote: invalid octal (only \o000 .. \o377 are allowed))"}};

        case ReasonEnum::INVALID_HEX:
            // Syntax allows possible future support for emojis or whatever,
            // but we currently only support LATIN1 characters.
            return UnquoteResult{UnquoteFailureReason{
                R"(unquote: invalid hex (only \x##, \u####, or \U######## are allowed))"}};
        case ReasonEnum::UNBALANCED_QUOTES:
            return UnquoteResult{UnquoteFailureReason{"unquote: unbalanced quotes"}};
        }
    } catch (const std::exception &ex) {
        return UnquoteResult{UnquoteFailureReason{std::string("exception: ") + ex.what()}};
    } catch (...) {
    }
    return UnquoteResult{UnquoteFailureReason{"unknown error"}};
}

void test::testUnquote()
{
    {
        static const auto expectString2 = [](const std::string_view input,
                                             const std::string_view expected,
                                             const bool allowUnbalancedQuotes) {
            auto result = unquote_unsafe(input, allowUnbalancedQuotes);
            TEST_ASSERT(result == std::vector<std::string>{std::string{expected}});
        };

        static const auto expectString = [](const std::string_view input,
                                            const std::string_view expected) {
            expectString2(input, expected, false);
        };

        expectString2("\"unbalanced", "unbalanced", true);
        expectString(R"("Hello, world!\n")", "Hello, world!\n");
        expectString(R"("\x10""ffff")",
                     "\x10"
                     "ffff");
        expectString(R"("\x10""ffff")",
                     "\u0010"
                     "ffff");
        expectString(R"("\x10""ffff")",
                     "\U00000010"
                     "ffff");
        expectString(R"("\u10ffff")", "?ff");
        expectString(R"("\U0010ffff")", "?");
        expectString(R"("a"b"c")", "abc");
        expectString(R"("foo""bar")", "foobar");
        expectString("\\n", "\\n");     // escapes outside of quotes are ignored
        expectString(R"(\"n")", "\\n"); // escapes outside of quotes are ignored
        expectString(R"(\""n)", "\\n"); // escapes outside of quotes are ignored

        // Rather than "too many hex digits", we just expect the transliteration + remainder.
        expectString(R"("\ufffff")", "?f");
        expectString(R"("\Ufffffffff")", "?f");

        using string_consts::S_ESC;
        expectString(R"("\e")", S_ESC);

        using char_consts::C_NUL;
        static constexpr const char NULL_CHAR_ARRAY[2] = {C_NUL, C_NUL};
        static constexpr const std::string_view NULL_CHAR_STRINGVIEW{NULL_CHAR_ARRAY, 1};
        static_assert(NULL_CHAR_STRINGVIEW.length() == 1);
        static_assert(NULL_CHAR_STRINGVIEW[0] == C_NUL);
        expectString(R"("\0")", NULL_CHAR_STRINGVIEW);
        expectString(R"("\o000")", NULL_CHAR_STRINGVIEW);

        expectString(R"("\o033")", S_ESC);  // C_ESC
        expectString(R"("\o377")", "\377"); // '\xFF'
    }
    {
        static const auto expectUnquoteException = [](const std::string_view input,
                                                      const ReasonEnum expectedReason) {
            try {
                MAYBE_UNUSED const auto ignored = //
                    unquote_unsafe(input, false);
                TEST_ASSERT(false);
            } catch (const UnquoteException &ex) {
                const auto &reason = ex.reason;
                TEST_ASSERT(reason == expectedReason);
            }
        };

        expectUnquoteException(R"("\x")", ReasonEnum::INVALID_HEX);
        expectUnquoteException(R"("\ufff")", ReasonEnum::INVALID_HEX);
        expectUnquoteException(R"("\Ufffffff")", ReasonEnum::INVALID_HEX);

        expectUnquoteException("\"unbalanced", ReasonEnum::UNBALANCED_QUOTES);
        expectUnquoteException(R"("a""b)", ReasonEnum::UNBALANCED_QUOTES);

        expectUnquoteException(R"("\1")", ReasonEnum::INVALID_ESCAPE);
        expectUnquoteException(R"("\o0")", ReasonEnum::INVALID_OCTAL);
        expectUnquoteException(R"("\o00")", ReasonEnum::INVALID_OCTAL);
        expectUnquoteException(R"("\o400")", ReasonEnum::INVALID_OCTAL);
        expectUnquoteException(R"("\o777")", ReasonEnum::INVALID_OCTAL);
    }

    {
        const VectorOfStrings expect = {"abc", "def", "ghi"};
        TEST_ASSERT(unquote_unsafe("abc def ghi", false) == expect);
    }
    {
        const VectorOfStrings expect = {"abc def", "ghi"};
        TEST_ASSERT(unquote_unsafe("ab\"c d\"ef ghi", false) == expect);
    }
    {
        const VectorOfStrings expect = {"abc def ghi"};
        TEST_ASSERT(unquote_unsafe("ab\"c def ghi", true) == expect);
    }
    {
        const VectorOfStrings expect = {"ab", "c def ghi"};
        TEST_ASSERT(unquote_unsafe("ab \"c def ghi", true) == expect);
    }

    {
        const VectorOfStrings expect = {"a", "b", "c"};
        TEST_ASSERT(unquote_unsafe("\t\t\ta \t b\r\nc\n", true) == expect);
    }

    {
        using char_consts::C_NUL;
        const char buf[8] = {'a', 'b', 'c', C_NUL, 'd', 'e', 'f', C_NUL};
        const std::string s{buf, 7};
        TEST_ASSERT(s.length() == 7);
        TEST_ASSERT(std::strlen(s.c_str()) == 3);
        const VectorOfStrings expect{s};
        const auto result = unquote(R"("abc\0def")", false, true);
        TEST_ASSERT(result && result.getVectorOfStrings() == expect);
    }
    {
        const VectorOfStrings expect = {"abc"};
        const auto result = unquote(R"("abc\0def")", false, false);
        TEST_ASSERT(result && result.getVectorOfStrings() == expect);
    }
}
