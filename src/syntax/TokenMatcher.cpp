// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TokenMatcher.h"

#include <cassert>
#include <iostream>
#include <utility>

#include "../global/TextUtils.h"

namespace syntax {

IMatchErrorLogger::~IMatchErrorLogger() = default;

MatchResult TokenMatcher::tryMatch(const ParserInput &sv, IMatchErrorLogger *logger) const
{
    return m_self->match(sv, logger);
}

std::ostream &operator<<(std::ostream &os, const TokenMatcher &token)
{
    // os << "TokenMatcher(";
    os << *token.m_self;
    // os << ")";
    return os;
}

} // namespace syntax
