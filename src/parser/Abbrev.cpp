// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Abbrev.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <QtCore>

#include "../global/StringView.h"
#include "../global/utils.h"

bool isAbbrev(StringView input, const char *const command, const int minAbbrev)
{
    const auto cmdLen = static_cast<int>(std::strlen(command));
    assert(minAbbrev == -1 || minAbbrev >= 1);
    assert(minAbbrev <= cmdLen);

    if (minAbbrev == -1 || minAbbrev == cmdLen)
        return input == command;

    int matched = 0;
    for (auto cmdIt = command; !input.isEmpty() && *cmdIt != '\0'; ++matched)
        if (std::tolower(input.takeFirstLetter()) != std::tolower(*cmdIt++))
            return false;

    if (!input.isEmpty())
        return false;

    return matched >= minAbbrev;
}

Abbrev::Abbrev(const char *const arg_command, const int arg_minAbbrev)
    : command{arg_command}
    , minAbbrev{arg_minAbbrev}
{
    if (this->command == nullptr || this->command[0] == '\0')
        throw std::invalid_argument("command");

    this->len = static_cast<int>(std::strlen(this->command));
    if (this->minAbbrev == -1)
        this->minAbbrev = this->len;

    if (!isClamped(this->minAbbrev, 1, this->len))
        throw std::invalid_argument("minAbbrev");
}

Abbrev::operator bool() const
{
    return command != nullptr && command[0] != '\0' && isClamped(minAbbrev, 1, len);
}

QString Abbrev::describe() const
{
    if (command == nullptr)
        return "";

    QByteArray result;
    result.reserve(len);
    for (int i = 0; i < len; ++i) {
        const char c = command[i];
        result.append(static_cast<char>((i < minAbbrev) ? std::toupper(c) : std::tolower(c)));
    }

    return result;
}
