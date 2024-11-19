// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Abbrev.h"

#include "../global/Consts.h"
#include "../global/StringView.h"
#include "../global/utils.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string_view>

#include <QtCore>

using char_consts::C_NUL;

bool isAbbrev(StringView input, const std::string_view command, const int minAbbrev)
{
    const auto cmdLen = static_cast<int>(command.length());
    assert(minAbbrev == -1 || minAbbrev >= 1);
    assert(minAbbrev <= cmdLen);

    if (minAbbrev == -1 || minAbbrev == cmdLen)
        return input.trim() == command;

    int matched = 0;
    for (const char c : command) {
        if (input.isEmpty())
            break;
        if (std::tolower(input.takeFirstLetter()) != std::tolower(c))
            return false;
        ++matched;
    }

    if (!input.isEmpty())
        return false;

    return matched >= minAbbrev;
}

Abbrev::Abbrev(const char *const arg_command, const int arg_minAbbrev)
    : m_command{arg_command}
    , m_minAbbrev{arg_minAbbrev}
{
    if (this->m_command == nullptr || this->m_command[0] == C_NUL)
        throw std::invalid_argument("command");

    this->m_len = static_cast<int>(std::strlen(this->m_command));
    if (this->m_minAbbrev == -1)
        this->m_minAbbrev = this->m_len;

    if (!isClamped(this->m_minAbbrev, 1, this->m_len))
        throw std::invalid_argument("minAbbrev");
}

Abbrev::operator bool() const
{
    return m_command != nullptr && m_command[0] != C_NUL && isClamped(m_minAbbrev, 1, m_len);
}

QString Abbrev::describe() const
{
    if (m_command == nullptr)
        return "";

    QByteArray result;
    result.reserve(m_len);
    for (int i = 0; i < m_len; ++i) {
        const char c = m_command[i];
        result.append(static_cast<char>((i < m_minAbbrev) ? std::toupper(c) : std::tolower(c)));
    }

    return result;
}
