/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "patterns.h"

#include <QRegExp>
#include <QStringList>

#include "../configuration/configuration.h"

QRegExp Patterns::g_rx;

const QRegExp Patterns::g_score(R"(\d+/\d+ hits(?:, \d+/\d+ mana,)? and \d+/\d+ moves.)");

bool Patterns::matchPattern(QString pattern, const QString &str)
{
    if (pattern.at(0) != '#') {
        return false;
    }

    switch (static_cast<int>((pattern.at(1)).toLatin1())) {
    case 33: // !
        g_rx.setPattern(pattern.remove(0, 2));
        if (g_rx.exactMatch(str)) {
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

bool Patterns::matchScore(const QString &str)
{
    return g_score.exactMatch(str);
}

bool Patterns::matchMoveForcePatterns(const QString &str)
{
    for (auto &pattern : Config().parser.moveForcePatternsList) {
        if (matchPattern(pattern, str)) {
            return true;
        }
    }
    return false;
}

bool Patterns::matchNoDescriptionPatterns(const QString &str)
{
    for (auto &pattern : Config().parser.noDescriptionPatternsList) {
        if (matchPattern(pattern, str)) {
            return true;
        }
    }
    return false;
}

bool Patterns::matchPasswordPatterns(const QByteArray &str)
{
    return matchPattern(Config().parser.passwordPattern, str);
}

bool Patterns::matchPromptPatterns(const QByteArray &str)
{
    return matchPattern(Config().parser.promptPattern, str);
}

bool Patterns::matchLoginPatterns(const QByteArray &str)
{
    return matchPattern(Config().parser.loginPattern, str);
}

bool Patterns::matchMenuPromptPatterns(const QByteArray &str)
{
    return matchPattern(Config().parser.menuPromptPattern, str);
}
