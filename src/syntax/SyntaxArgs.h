#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Consts.h"
#include "IArgument.h"
#include "TokenMatcher.h"

#include <optional>
#include <string>
#include <vector>

namespace syntax {

class NODISCARD ArgAbbrev final : public IArgument
{
private:
    std::string m_str;

public:
    explicit ArgAbbrev(std::string moved_str);

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgBool final : public IArgument
{
public:
    ArgBool() = default;

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgChoice final : public IArgument
{
private:
    std::vector<TokenMatcher> m_tokens;
    bool justPassResult = true;

public:
    explicit ArgChoice(std::vector<TokenMatcher> tokens);

    template<typename... Ts>
    explicit ArgChoice(TokenMatcher t1, TokenMatcher t2, Ts... ts)
        : m_tokens{std::move(t1), std::move(t2), std::move(ts)...}
    {}

private:
    NODISCARD MatchResult virt_match(const ParserInput &sv,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgInt final : public IArgument
{
private:
    std::optional<int> min;
    std::optional<int> max;

public:
    ArgInt() = default;

    NODISCARD static ArgInt withMax(int n);
    NODISCARD static ArgInt withMin(int n);
    NODISCARD static ArgInt withMinMax(int min, int max);

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgFloat final : public IArgument
{
private:
    std::optional<float> min;
    std::optional<float> max;

public:
    ArgFloat() = default;

    NODISCARD static ArgFloat withMax(float n);
    NODISCARD static ArgFloat withMin(float n);
    NODISCARD static ArgFloat withMinMax(float min, float max);

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgOneOrMoreToken final : public IArgument
{
private:
    TokenMatcher m_token;

public:
    explicit ArgOneOrMoreToken(TokenMatcher t)
        : m_token(std::move(t))
    {}

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

// always ignored.
class NODISCARD ArgOptionalChar final : public IArgument
{
private:
    char m_c = char_consts::C_NUL;

public:
    explicit ArgOptionalChar(char c)
        : m_c(c)
    {}

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgOptionalToken final : public IArgument
{
private:
    TokenMatcher m_token;
    bool m_ignored = false;

public:
    explicit ArgOptionalToken(TokenMatcher t)
        : m_token(std::move(t))
    {}

    NODISCARD static ArgOptionalToken ignored(TokenMatcher t)
    {
        ArgOptionalToken result{std::move(t)};
        result.m_ignored = true;
        return result;
    }

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

/// Matches 0 or more arguments; result is a Vector of raw (unquoted) strings.
///
/// ArgRest is effectively the same as arg_rest in MUME's mudlle.
class NODISCARD ArgRest final : public IArgument
{
private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

/// Matches exactly one argument; result is a raw (unquoted) string.
///
/// ArgString is effectively the same as arg_string in MUME's mudlle.
///
/// Be aware that ArgString allows whitespace in the token parsed by unquote.
/// e.g. If the ParserInput is ["two words", "etc"], ArgString will match "two words".
class NODISCARD ArgString final : public IArgument
{
public:
    ArgString() = default;

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgStringExact final : public IArgument
{
private:
    std::string m_str;

public:
    explicit ArgStringExact(std::string moved_str)
        : m_str(std::move(moved_str))
    {}

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class NODISCARD ArgStringIgnoreCase final : public IArgument
{
private:
    std::string m_str;

public:
    explicit ArgStringIgnoreCase(std::string moved_str)
        : m_str(std::move(moved_str))
    {}

private:
    NODISCARD MatchResult virt_match(const ParserInput &input,
                                     IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

/// matches the any non-zero abbreviation (case insensitive)
NODISCARD TokenMatcher abbrevToken(std::string s);
/// matches the entire string (case insensitive)
NODISCARD TokenMatcher stringToken(std::string s);

} // namespace syntax
