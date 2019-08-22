#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

bool isOffline();
bool isOnline();
const char *enabledString(bool isEnabled);
bool isValidPrefix(char c);
