#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <string>

#include "../syntax/Value.h"

NODISCARD bool isOffline();
NODISCARD bool isOnline();
NODISCARD const char *enabledString(bool isEnabled);
NODISCARD bool isValidPrefix(char c);

template<typename T>
void send_ok(T &os)
{
    // Consider changing this from "OK." to "Ok.", since that's what MUME uses.
    // Consider fixing the output to not require '\r'.
    os << "OK.\r\n";
}

NODISCARD std::string concatenate_unquoted(const Vector &input);
