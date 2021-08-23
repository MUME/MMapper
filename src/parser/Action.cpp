// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Action.h"

#include <regex>

IAction::~IAction() = default;

void IAction::match(const StringView &input) const
{
    return virt_match(input);
}

void StartsWithAction::virt_match(const StringView &input) const
{
    if (input.startsWith(match))
        callback(input);
}

void EndsWithAction::virt_match(const StringView &input) const
{
    if (input.endsWith(match))
        callback(input);
}

NODISCARD static std::regex createRegex(const std::string &pattern)
{
    return std::regex(pattern, std::regex::nosubs | std::regex::optimize);
}

RegexAction::RegexAction(const std::string &pattern, const ActionCallback &callback)
    : regex{createRegex(pattern)}
    , callback{callback}
{}

void RegexAction::virt_match(const StringView &input) const
{
    if (std::regex_match(input.begin(), input.end(), regex))
        callback(input);
}
