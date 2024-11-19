#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ParserInput.h"
#include "Value.h"

namespace syntax {

struct NODISCARD MatchResult final
{
public:
    ParserInput matched;
    ParserInput unmatched;
    OptValue optValue;
    bool is_success = true;

public:
    MatchResult() = delete;
    NODISCARD static MatchResult failure(ParserInput unmatched);
    // partial match
    NODISCARD static MatchResult failure(ParserInput matched, ParserInput unmatched);

public:
    NODISCARD static MatchResult success(size_t numMatched, const ParserInput &input, Value result);
    NODISCARD static MatchResult success(size_t numMatched, const ParserInput &input);
    // matched everything
    NODISCARD static MatchResult success(ParserInput input, OptValue optResult);

private:
    NODISCARD static MatchResult success(ParserInput matched,
                                         ParserInput unmatched,
                                         OptValue optResult);

private:
    MatchResult(ParserInput matched, ParserInput unmatched, OptValue optResult);

public:
    NODISCARD explicit operator bool() const { return is_success; }
};
} // namespace syntax
