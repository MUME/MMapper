// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "utils.h"

#include <cassert>
#include <cmath>
#include <limits>

int utils::round_ftoi(const float f)
{
    using lim = std::numeric_limits<int>;
    assert(std::isfinite(f));
    const long l = std::lround(f);
    assert(isClamped<long>(l, lim::min(), lim::max()));
    return static_cast<int>(l);
}
