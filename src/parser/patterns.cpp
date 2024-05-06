// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "patterns.h"

#include "../configuration/configuration.h"

#include <QRegularExpression>

bool Patterns::matchPattern(const QString &pattern, const QString &str)
{
    if (pattern.at(0) != '#') {
        return false;
    }

    switch (static_cast<int>((pattern.at(1)).toLatin1())) {
    case 33: // !
        if (QRegularExpression(pattern.mid(2)).match(str).hasMatch()) {
            return true;
        }
        break;
    case 60:; // <
        if (str.startsWith(pattern.mid(2))) {
            return true;
        }
        break;
    case 61:; // =
        if (str == (pattern.mid(2))) {
            return true;
        }
        break;
    case 62:; // >
        if (str.endsWith(pattern.mid(2))) {
            return true;
        }
        break;
    case 63:; // ?
        if (str.contains(pattern.mid(2))) {
            return true;
        }
        break;
    default:;
    }
    return false;
}

bool Patterns::matchNoDescriptionPatterns(const QString &str)
{
    for (const QString &pattern : getConfig().parser.noDescriptionPatternsList) {
        if (matchPattern(pattern, str)) {
            return true;
        }
    }
    return false;
}
