// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "patterns.h"

#include <QRegularExpression>

#include "../configuration/configuration.h"

bool Patterns::matchPattern(QString pattern, const QString &str)
{
    if (pattern.at(0) != '#') {
        return false;
    }

    switch (static_cast<int>((pattern.at(1)).toLatin1())) {
    case 33: // !
        if (QRegularExpression(pattern.remove(0, 2)).match(str).hasMatch()) {
            return true;
        }
        break;
    case 60:; // <
        if (str.startsWith(pattern.remove(0, 2))) {
            return true;
        }
        break;
    case 61:; // =
        if (str == (pattern.remove(0, 2))) {
            return true;
        }
        break;
    case 62:; // >
        if (str.endsWith(pattern.remove(0, 2))) {
            return true;
        }
        break;
    case 63:; // ?
        if (str.contains(pattern.remove(0, 2))) {
            return true;
        }
        break;
    default:;
    }
    return false;
}

bool Patterns::matchPattern(QByteArray pattern, const QByteArray &str)
{
    if (pattern.at(0) != '#') {
        return false;
    }

    switch (static_cast<int>(pattern.at(1))) {
    case 33: // !
        break;
    case 60:; // <
        if (str.startsWith(pattern.remove(0, 2))) {
            return true;
        }
        break;
    case 61:; // =
        if (str == (pattern.remove(0, 2))) {
            return true;
        }
        break;
    case 62:; // >
        if (str.endsWith(pattern.remove(0, 2))) {
            return true;
        }
        break;
    case 63:; // ?
        if (str.contains(pattern.remove(0, 2))) {
            return true;
        }
        break;
    default:;
    }
    return false;
}

bool Patterns::matchNoDescriptionPatterns(const QString &str)
{
    for (auto &pattern : getConfig().parser.noDescriptionPatternsList) {
        if (matchPattern(pattern, str)) {
            return true;
        }
    }
    return false;
}

bool Patterns::matchPasswordPatterns(const QByteArray &str)
{
    static const QRegularExpression re(R"(^Account pass phrase:? $)");
    return re.match(str).hasMatch();
}

bool Patterns::matchLoginPatterns(const QByteArray &str)
{
    static const QRegularExpression re(R"(^By what name do you wish to be known\?? $)");
    return re.match(str).hasMatch();
}

bool Patterns::matchMenuPromptPatterns(const QByteArray &str)
{
    static const QRegularExpression re(R"(^Account(>|&gt;)? $)");
    return re.match(str).hasMatch();
}
