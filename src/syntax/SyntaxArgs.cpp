// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "SyntaxArgs.h"

#include "../global/CaseUtils.h"
#include "../global/Charset.h"
#include "../global/PrintUtils.h"
#include "IMatchErrorLogger.h"
#include "ParserInput.h"
#include "TokenMatcher.h"

#include <cmath>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace { // anonymous
NODISCARD bool compareIgnoreCase(const std::string_view &a, const std::string_view &b)
{
    return areEqualAsLowerUtf8(a, b);
}
} // namespace

namespace syntax {

NODISCARD static Vector toVector(const ParserInput &input)
{
    std::vector<Value> values;
    values.reserve(input.size());
    for (const std::string &s : input) {
        values.emplace_back(s);
    }
    return Vector{std::exchange(values, {})};
}

ArgAbbrev::ArgAbbrev(std::string moved_str)
    : m_str(std::move(moved_str))
{
    for (const char c : m_str) {
        // use nbsp_aware::isAnySpace?
        // Note: This currently forbids non-latin1 codepoints.
        if (ascii::isSpace(c) || c == char_consts::C_NBSP || !isPrintLatin1(c)) {
            throw std::invalid_argument("string");
        }
    }
}

MatchResult ArgAbbrev::virt_match(const ParserInput &input, IMatchErrorLogger * /*logger*/) const
{
    if (input.empty()) {
        return MatchResult::failure(input);
    }

    const auto &input_sv = input.front();
    const size_t inputLen = input_sv.size();
    if (inputLen > m_str.length()) {
        return MatchResult::failure(input);
    }

    // Note: If this ever contains non-latin1 codepoints, then substr() could slice a codepoint,
    // which could then allow "???" to match the sliced codepoint.
    const auto maybe_sliced = std::string_view{m_str}.substr(0, inputLen);
    if (!isValidUtf8(maybe_sliced) || !compareIgnoreCase(maybe_sliced, input_sv)) {
        return MatchResult::failure(input);
    }

    return MatchResult::success(1, input, Value{m_str});
}

std::ostream &ArgAbbrev::virt_to_stream(std::ostream &os) const
{
    return print_string_smartquote(os, toLowerUtf8(m_str));
}

MatchResult ArgBool::virt_match(const syntax::ParserInput &input,
                                syntax::IMatchErrorLogger * /*logger*/) const
{
    if (input.empty()) {
        return MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);
    const auto first = toLowerUtf8(input.front());

    if (first == "true" || first == "yes" || first == "1") {
        return MatchResult::success(1, input, Value{true});
    } else if (first == "false" || first == "no" || first == "0") {
        return MatchResult::success(1, input, Value{false});
    }

    return MatchResult::failure(input);
}

std::ostream &ArgBool::virt_to_stream(std::ostream &os) const
{
    return os << "<bool>";
}

ArgChoice::ArgChoice(std::vector<TokenMatcher> tokens)
    : m_tokens(std::move(tokens))
{
    if (m_tokens.size() < 2) {
        throw std::runtime_error("choice must have at least two elements");
    }
}

MatchResult ArgChoice::virt_match(const ParserInput &input_sv, IMatchErrorLogger *logger) const
{
    auto &arg = *this;
    ParserInput best = input_sv.left(0);
    int n = -1;
    for (auto &token : arg.m_tokens) {
        ++n;
        if (MatchResult result = token.tryMatch(input_sv, logger)) {
            if (arg.justPassResult) {
                return result;
            }

            std::vector<Value> v;
            v.emplace_back(n);
            if (result.optValue) {
                v.emplace_back(result.optValue.value());
            }

            /* replace the value */
            result.optValue = Value(Vector(std::move(v)));
            return result;
        } else {
            if (result.matched.length() > best.length()) {
                best = result.matched;
            }
        }
    }

    return MatchResult::failure(best, input_sv.mid(best.length()));
}

std::ostream &ArgChoice::virt_to_stream(std::ostream &os) const
{
    os << "<";
    bool first = true;
    for (auto &token : m_tokens) {
        if (first) {
            first = false;
        } else {
            os << "|";
        }
        os << token;
    }
    os << ">";
    return os;
}

ArgInt ArgInt::withMax(int n)
{
    ArgInt result;
    result.range.max = n;
    return result;
}

ArgInt ArgInt::withMin(int n)
{
    ArgInt result;
    result.range.min = n;
    return result;
}

ArgInt ArgInt::withMinMax(int min, int max)
{
    if (min > max) {
        throw std::invalid_argument("max");
    }
    ArgInt result;
    result.range.min = min;
    result.range.max = max;
    return result;
}

std::optional<int> tryMatchInt(StringView input_sv,
                               const IntRange &range,
                               IMatchErrorLogger *const logger)
{
    // early validation
    {
        auto stringView = input_sv;

        // The function std::stoi is documented to skip leading whitespace.
        const char firstChar = stringView.firstChar();
        if (!std::isdigit(firstChar) && firstChar != char_consts::C_MINUS_SIGN
            && firstChar != char_consts::C_PLUS_SIGN) {
            return std::nullopt;
        }

        ++stringView;
        for (char c : stringView) {
            if (!std::isdigit(c)) {
                return std::nullopt;
            }
        }

        if (!std::isdigit(input_sv.lastChar())) {
            return std::nullopt;
        }
    }

    using Limits = std::numeric_limits<int>;
    const int minVal = range.min.value_or(Limits::min());
    const int maxVal = range.max.value_or(Limits::max());

    auto sv = input_sv;
    bool negative = false;
    switch (sv.firstChar()) {
    case char_consts::C_PLUS_SIGN:
        ++sv;
        break;
    case char_consts::C_MINUS_SIGN:
        ++sv;
        negative = true;
        break;
    default:
        break;
    }

    constexpr bool reportPartialInteger = false;

    bool fail = false;
    int64_t result = 0;
    for (; !sv.isEmpty(); ++sv) {
        const char c = sv.firstChar();
        assert(std::isdigit(c));
        const int n = c - '0';
        result *= 10;
        result += n;
        if (negative) {
            if (-result < minVal) {
                if (logger) {
                    const auto reported = reportPartialInteger ? input_sv.beforeSubstring(sv.mid(1))
                                                               : input_sv;
                    logger->logError("input " + reported.toStdString() + " is less than "
                                     + std::to_string(minVal));
                }
                fail = true;
                break;
            }
        } else {
            if (result > maxVal) {
                if (logger) {
                    const auto reported = reportPartialInteger ? input_sv.beforeSubstring(sv.mid(1))
                                                               : input_sv;
                    logger->logError("input " + reported.toStdString() + " is greater than "
                                     + std::to_string(maxVal));
                }
                fail = true;
                break;
            }
        }
    }

    if (negative) {
        result = -result;
    }

    if (fail) {
        return std::nullopt;
    }

    return static_cast<int>(result);
}

MatchResult ArgInt::virt_match(const ParserInput &input, IMatchErrorLogger *logger) const
{
    // MAYBE_UNUSED auto &arg = *this;
    if (input.empty()) {
        return MatchResult::failure(input);
    }

    const auto input_sv = StringView{input.front()};

    if (auto optInt = syntax::tryMatchInt(input_sv, range, logger)) {
        return MatchResult::success(1, input, Value{optInt.value()});
    }

    return MatchResult::failure(input);
}

std::ostream &ArgInt::virt_to_stream(std::ostream &os) const
{
    const bool hasMin = range.min.has_value();
    const bool hasMax = range.max.has_value();
    const bool singleValue = (hasMin && hasMax && range.min == range.max);

    os << "<integer";

    if (hasMin || hasMax) {
        os << ":";
    }

    if (hasMin) {
        os << " " << range.min.value();
    }

    if (!singleValue) {
        if (hasMin || hasMax) {
            os << " ..";
        }

        if (hasMax) {
            os << " " << range.max.value();
        }
    }

    return os << ">";
}

ArgFloat ArgFloat::withMax(float n)
{
    ArgFloat result;
    result.max = n;
    return result;
}

ArgFloat ArgFloat::withMin(float n)
{
    ArgFloat result;
    result.min = n;
    return result;
}

ArgFloat ArgFloat::withMinMax(float min, float max)
{
    if (!std::isfinite(min)) {
        throw std::invalid_argument("min");
    }
    if (!std::isfinite(max) || min > max) {
        throw std::invalid_argument("max");
    }
    ArgFloat result;
    result.min = min;
    result.max = max;
    return result;
}

MatchResult ArgFloat::virt_match(const ParserInput &input, IMatchErrorLogger *logger) const
{
    auto &arg = *this;
    if (input.empty()) {
        return MatchResult::failure(input);
    }

    const std::string &firstWord = input.front();
    using Limits = std::numeric_limits<float>;
    const float minVal = arg.min.value_or(Limits::min());
    const float maxVal = arg.max.value_or(Limits::max());

    char *end = nullptr;
    const auto beg = firstWord.data();
    const float f = std::strtof(beg, &end);

    if (!std::isfinite(f) || (utils::equals(f, 0.f) && end == beg)
        || end != beg + firstWord.size()) {
        return MatchResult::failure(input);
    }

    if (f < minVal) {
        if (logger) {
            logger->logError("input " + firstWord + " is less than " + std::to_string(minVal));
        }
        return MatchResult::failure(input);
    }

    if (f > maxVal) {
        if (logger) {
            logger->logError("input " + firstWord + " is greater than " + std::to_string(maxVal));
        }
        return MatchResult::failure(input);
    }

    return MatchResult::success(1, input, Value(f));
}

std::ostream &ArgFloat::virt_to_stream(std::ostream &os) const
{
    auto &arg = *this;
    const bool hasMin = arg.min.has_value();
    const bool hasMax = arg.max.has_value();
    const bool singleValue = (hasMin && hasMax && arg.min == arg.max);

    os << "<float";

    if (hasMin || hasMax) {
        os << ":";
    }

    if (hasMin) {
        os << " " << arg.min.value();
    }

    if (!singleValue) {
        if (hasMin || hasMax) {
            os << " ..";
        }

        if (hasMax) {
            os << " " << arg.max.value();
        }
    }

    return os << ">";
}

MatchResult ArgOneOrMoreToken::virt_match(const ParserInput &input, IMatchErrorLogger *logger) const
{
    auto &arg = *this;
    std::vector<Value> values;
    auto current = input;
    while (!current.empty()) {
        MatchResult result = arg.m_token.tryMatch(current.left(1), logger);
        if (!result) {
            break;
        }
        values.emplace_back(result.optValue.value_or(Value{}));
        current = current.mid(1);
    }
    if (values.empty()) {
        return MatchResult::failure(input);
    }
    const auto size = values.size();
    return MatchResult::success(size, input, Value(Vector(std::move(values))));
}

std::ostream &ArgOneOrMoreToken::virt_to_stream(std::ostream &os) const
{
    return os << "[" << m_token << "]...";
}

MatchResult ArgOptionalChar::virt_match(const ParserInput &input,
                                        IMatchErrorLogger * /*logger*/) const
{
    size_t matched = 0;
    if (!input.empty() && input.front().size() == 1 && input.front().front() == m_c) {
        ++matched;
    }

    return MatchResult::success(matched, input);
}

std::ostream &ArgOptionalChar::virt_to_stream(std::ostream &os) const
{
    os << "[";
    os << SmartQuotedString(std::string(1, m_c));
    os << "]";
    return os;
}

MatchResult ArgOptionalToken::virt_match(const ParserInput &input_sv,
                                         IMatchErrorLogger *logger) const
{
    auto &arg = *this;
    if (MatchResult result = arg.m_token.tryMatch(input_sv, logger)) {
        if (arg.m_ignored) {
            result.optValue.reset(); /* ignored! */
            return result;
        }

        std::vector<Value> v;
        v.emplace_back(Value(true));
        if (result.optValue) {
            v.emplace_back(result.optValue.value());
        }

        // replace the value
        result.optValue = Value(Vector(std::move(v)));
        return result;
    } else {
        // NOTE: It's not possible to use the result's failing partial match here,
        // because we're claiming success. This means that the parser help function
        // cannot report partial match of optional arguments.
        if (arg.m_ignored) {
            return MatchResult::success(0, input_sv);
        } else {
            return MatchResult::success(0, input_sv, Value(false));
        }
    }
}

std::ostream &ArgOptionalToken::virt_to_stream(std::ostream &os) const
{
    if (m_ignored) {
        os << "ignored";
    }
    return os << "[" << m_token << "]";
}

MatchResult ArgRest::virt_match(const ParserInput &input, IMatchErrorLogger * /*logger*/) const
{
    return MatchResult::success(input, Value{toVector(input)});
}

std::ostream &ArgRest::virt_to_stream(std::ostream &os) const
{
    return os << "[...]";
}

MatchResult ArgString::virt_match(const ParserInput &input, IMatchErrorLogger * /*logger*/) const
{
    if (input.length() != 1) {
        return MatchResult::failure(input);
    }

    return MatchResult::success(1, input, Value{input.front()});
}

std::ostream &ArgString::virt_to_stream(std::ostream &os) const
{
    return os << "<string>";
}

MatchResult ArgStringExact::virt_match(const ParserInput &input,
                                       IMatchErrorLogger * /*logger*/) const
{
    if (input.empty() || m_str != input.front()) {
        return MatchResult::failure(input);
    }

    return MatchResult::success(1, input);
}

std::ostream &ArgStringExact::virt_to_stream(std::ostream &os) const
{
    os << "<EXACT: ";
    return print_string_quoted(os, m_str) << ">";
}

MatchResult ArgStringIgnoreCase::virt_match(const ParserInput &input,
                                            IMatchErrorLogger * /*logger*/) const
{
    if (input.empty() || !compareIgnoreCase(m_str, input.front())) {
        return MatchResult::failure(input);
    }

    return MatchResult::success(1, input);
}

std::ostream &ArgStringIgnoreCase::virt_to_stream(std::ostream &os) const
{
    return print_string_smartquote(os, toLowerUtf8(m_str));
}

TokenMatcher abbrevToken(std::string s)
{
    return TokenMatcher::alloc<ArgAbbrev>(std::move(s));
}
TokenMatcher stringToken(std::string s)
{
    return TokenMatcher::alloc<ArgStringIgnoreCase>(std::move(s));
}

syntax::MatchResult ArgHexColor::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    auto arg = StringView{input.front()};
    if (!arg.startsWith("#")) {
        return syntax::MatchResult::failure(input);
    }

    ++arg;

    for (const char c : arg) {
        if (!std::isxdigit(c)) {
            return syntax::MatchResult::failure(input);
        }
    }

    if (arg.length() != 6) {
        return syntax::MatchResult::failure(input);
    }

    const auto color = Color::fromHex(arg.getStdStringView());
    return syntax::MatchResult::success(1, input, Value{static_cast<int64_t>(color.getRGB())});
}

std::ostream &ArgHexColor::virt_to_stream(std::ostream &os) const
{
    return os << "<HexColor>";
}
} // namespace syntax
