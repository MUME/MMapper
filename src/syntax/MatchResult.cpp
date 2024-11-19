// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "MatchResult.h"

namespace syntax {

MatchResult::MatchResult(ParserInput matched, ParserInput unmatched, OptValue optResult)
    : matched(std::move(matched))
    , unmatched(std::move(unmatched))
    , optValue(std::move(optResult))
{}

MatchResult MatchResult::failure(ParserInput unmatched)
{
    auto left = unmatched.left(0);
    return failure(std::move(left), std::move(unmatched));
}

MatchResult MatchResult::failure(ParserInput matched, ParserInput unmatched)
{
    MatchResult result{std::move(matched), std::move(unmatched), std::nullopt};
    result.is_success = false;
    return result;
}

MatchResult MatchResult::success(ParserInput matched, ParserInput unmatched, OptValue optResult)
{
    return MatchResult{std::move(matched), std::move(unmatched), std::move(optResult)};
}

MatchResult MatchResult::success(size_t numMatched, const ParserInput &input, Value result)
{
    return success(input.left(numMatched), input.mid(numMatched), std::move(result));
}

MatchResult MatchResult::success(size_t numMatched, const ParserInput &input)
{
    return success(input.left(numMatched), input.mid(numMatched), std::nullopt);
}

MatchResult MatchResult::success(ParserInput input, OptValue optResult)
{
    auto right = input.right(0);
    return success(std::move(input), std::move(right), std::move(optResult));
}

} // namespace syntax
