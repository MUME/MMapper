#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../syntax/Value.h"

#include <string>

NODISCARD bool isOffline();
NODISCARD bool isOnline();
NODISCARD const char *enabledString(bool isEnabled);
NODISCARD bool isValidPrefix(char c);

template<typename T>
void send_ok(T &os)
{
    // Note: MUME uses "Ok." rahter than "OK."
    os << "Ok.\n";
}

NODISCARD std::string concatenate_unquoted(const Vector &input);
