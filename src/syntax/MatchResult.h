#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ParserInput.h"
#include "Value.h"

namespace syntax {

struct MatchResult final
{
public:
    ParserInput matched;
    ParserInput unmatched;
    OptValue optValue;
    bool is_success = true;

public:
    MatchResult() = delete;
    static MatchResult failure(ParserInput unmatched);
    // partial match
    static MatchResult failure(ParserInput matched, ParserInput unmatched);

public:
    static MatchResult success(size_t numMatched, ParserInput input, Value result);
    static MatchResult success(size_t numMatched, ParserInput input);
    // matched everything
    static MatchResult success(ParserInput input, OptValue optResult);

private:
    static MatchResult success(ParserInput matched, ParserInput unmatched, OptValue optResult);

private:
    MatchResult(ParserInput matched, ParserInput unmatched, OptValue optResult);

public:
    explicit operator bool() const { return is_success; }
};
} // namespace syntax
