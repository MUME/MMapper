#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <cstdint>
#include <functional>

class Coordinate;

enum class NODISCARD FindCoordEnum : uint8_t { InUse, Available };
NODISCARD extern Coordinate getNearestFree(
    const Coordinate &c, const std::function<FindCoordEnum(const Coordinate &)> &check);
