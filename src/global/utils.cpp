// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "utils.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <QByteArray>
#include <qglobal.h>

int utils::round_ftoi(const float f)
{
    using lim = std::numeric_limits<int>;
    assert(std::isfinite(f));
    const long l = std::lround(f);
    assert(isClamped<long>(l, lim::min(), lim::max()));
    return static_cast<int>(l);
}

std::optional<bool> utils::getEnvBool(const char *const key)
{
    if (!qEnvironmentVariableIsSet(key))
        return std::nullopt;

    bool valid = false;
    const int value = qEnvironmentVariableIntValue(key, &valid);
    if (valid)
        return value == 1;

    QByteArray arr = qgetenv(key).toLower();
    if (arr == "true" || arr == "yes")
        return true;
    else if (arr == "false" || arr == "no")
        return false;

    return std::nullopt;
}

std::optional<int> utils::getEnvInt(const char *const key)
{
    if (!qEnvironmentVariableIsSet(key))
        return std::nullopt;

    bool valid = false;
    const int value = qEnvironmentVariableIntValue(key, &valid);
    if (!valid)
        return std::nullopt;

    return value;
}
