#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

bool isOffline();
bool isOnline();
const char *enabledString(bool isEnabled);
bool isValidPrefix(char c);

template<typename T>
void send_ok(T &os)
{
    // Consider changing this from "OK." to "Ok.", since that's what MUME uses.
    // Consider fixing the output to not require '\r'.
    os << "OK.\r\n";
}
