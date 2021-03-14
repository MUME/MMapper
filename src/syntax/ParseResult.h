#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <variant>

#include "MatchResult.h"
#include "ParserInput.h"

namespace syntax {

struct NODISCARD Success final
{
    ParserInput matched;
    ParserInput unmatched;

    explicit Success(ParserInput moved_matched)
        : matched(std::move(moved_matched))
        , unmatched(this->matched.right(0))
    {}
};

struct NODISCARD ParseResult final : public std::variant<Success, MatchResult>
{
    using Base = std::variant<Success, MatchResult>;

public:
    NODISCARD bool isSuccess() const { return std::holds_alternative<Success>(*this); }
    NODISCARD bool isFailure() const
    {
        if (isSuccess())
            return false;
        assert(std::holds_alternative<MatchResult>(*this));
        const auto &result = std::get<MatchResult>(*this);
        return !result;
    }

    /* implicit */ ParseResult(Success success)
        : Base{std::move(success)}
    {}
    /* implicit */ ParseResult(MatchResult result)
        : Base{std::move(result)}
    {}

    NODISCARD static ParseResult success(ParserInput matched)
    {
        return ParseResult{Success(std::move(matched))};
    }

    NODISCARD static ParseResult failure(ParserInput unmatched)
    {
        return ParseResult{MatchResult::failure(std::move(unmatched))};
    }
};

} // namespace syntax
