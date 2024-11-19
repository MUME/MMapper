// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "patterns.h"

#include "../configuration/configuration.h"
#include "../global/Consts.h"

#include <QRegularExpression>

bool Patterns::matchPattern(const QString &pattern, const QString &str)
{
    using namespace char_consts;
    if (pattern.at(0) != C_POUND_SIGN) {
        return false;
    }

    switch (static_cast<int>((pattern.at(1)).toLatin1())) {
    case C_EXCLAMATION: // !
        if (QRegularExpression(pattern.mid(2)).match(str).hasMatch()) {
            return true;
        }
        break;
    case C_LESS_THAN: // <
        if (str.startsWith(pattern.mid(2))) {
            return true;
        }
        break;
    case C_EQUALS: // =
        if (str == (pattern.mid(2))) {
            return true;
        }
        break;
    case C_GREATER_THAN: // >
        if (str.endsWith(pattern.mid(2))) {
            return true;
        }
        break;
    case C_QUESTION_MARK: // ?
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
