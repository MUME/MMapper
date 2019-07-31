// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractParser-Utils.h"

#include <cctype>

#include "../configuration/configuration.h"

bool isOffline()
{
    return getConfig().general.mapMode == MapModeEnum::OFFLINE;
}

bool isOnline()
{
    return !isOffline();
}

const char *enabledString(const bool isEnabled)
{
    return isEnabled ? "enabled" : "disabled";
}

bool isValidPrefix(const char c)
{
    return std::ispunct(c);
}
