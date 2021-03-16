// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "IArgument.h"

namespace syntax {

IArgument::~IArgument() = default;

MatchResult IArgument::match(const ParserInput &input, IMatchErrorLogger *logger) const
{
    return virt_match(input, logger);
}

std::ostream &IArgument::to_stream(std::ostream &os) const
{
    return virt_to_stream(os);
}

std::ostream &operator<<(std::ostream &os, const IArgument &arg)
{
    return arg.to_stream(os);
}

} // namespace syntax
