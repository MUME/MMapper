// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ServerIdMap.h"

#include "../global/AnsiOstream.h"

void ServerIdMap::printStats(ProgressCounter & /*pc*/, AnsiOstream &os) const
{
    static constexpr auto green = getRawAnsi(AnsiColor16Enum::green);
    os << "Unique server ids assigned: " << ColoredValue{green, this->size()} << ".\n";
}
