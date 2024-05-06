#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "MatchResult.h"

#include <iosfwd>

namespace syntax {

struct IMatchErrorLogger;
class ParserInput;

struct NODISCARD IArgument
{
public:
    virtual ~IArgument();
    IArgument() = default;
    DEFAULT_CTORS_AND_ASSIGN_OPS(IArgument);

private:
    NODISCARD virtual MatchResult virt_match(const ParserInput &input,
                                             IMatchErrorLogger *logger) const = 0;
    virtual std::ostream &virt_to_stream(std::ostream &) const = 0;

public:
    NODISCARD MatchResult match(const ParserInput &input, IMatchErrorLogger *logger) const;
    std::ostream &to_stream(std::ostream &os) const;

public:
    friend std::ostream &operator<<(std::ostream &os, const IArgument &arg);
};

} // namespace syntax
