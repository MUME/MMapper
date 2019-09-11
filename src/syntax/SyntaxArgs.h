#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <optional>
#include <string>
#include <vector>

#include "IArgument.h"
#include "TokenMatcher.h"

namespace syntax {

class ArgAbbrev final : public IArgument
{
public:
    std::string m_str;

public:
    explicit ArgAbbrev(std::string moved_str);

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class ArgBool final : public IArgument
{
public:
    ArgBool() = default;

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class ArgChoice final : public IArgument
{
public:
    std::vector<TokenMatcher> m_tokens;
    bool justPassResult = true;

public:
    explicit ArgChoice(std::vector<TokenMatcher> tokens);

    template<typename... Ts>
    explicit ArgChoice(TokenMatcher t1, TokenMatcher t2, Ts... ts)
        : m_tokens{std::move(t1), std::move(t2), std::move(ts)...}
    {}

private:
    MatchResult virt_match(const ParserInput &sv, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class ArgInt final : public IArgument
{
public:
    std::optional<int> min;
    std::optional<int> max;

public:
    ArgInt() = default;

    static ArgInt withMax(int n);
    static ArgInt withMin(int n);
    static ArgInt withMinMax(int min, int max);

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class ArgFloat final : public IArgument
{
public:
    std::optional<float> min;
    std::optional<float> max;

public:
    ArgFloat() = default;

    static ArgFloat withMax(float n);
    static ArgFloat withMin(float n);
    static ArgFloat withMinMax(float min, float max);

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

// always ignored.
struct ArgOptionalChar final : public IArgument
{
private:
    char m_c = '\0';

public:
    explicit ArgOptionalChar(char c)
        : m_c(c)
    {}

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class ArgOptionalToken final : public IArgument
{
private:
    TokenMatcher m_token;
    bool m_ignored = false;

public:
    explicit ArgOptionalToken(TokenMatcher t)
        : m_token(std::move(t))
    {}

    static ArgOptionalToken ignored(TokenMatcher t)
    {
        ArgOptionalToken result{std::move(t)};
        result.m_ignored = true;
        return result;
    }

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

class ArgRest final : public IArgument
{
private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

struct ArgStringExact final : public IArgument
{
private:
    std::string m_str;

public:
    explicit ArgStringExact(std::string moved_str)
        : m_str(std::move(moved_str))
    {}

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

struct ArgStringIgnoreCase final : public IArgument
{
private:
    std::string m_str;

public:
    explicit ArgStringIgnoreCase(std::string moved_str)
        : m_str(std::move(moved_str))
    {}

private:
    MatchResult virt_match(const ParserInput &input, IMatchErrorLogger *logger) const override;
    std::ostream &virt_to_stream(std::ostream &os) const override;
};

/// matches the any non-zero abbreviation (case insensitive)
TokenMatcher abbrevToken(std::string s);
/// matches the entire string (case insensitive)
TokenMatcher stringToken(std::string s);

} // namespace syntax
