// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "parserutils.h"

#include <QRegularExpression>
#include <QtCore>

namespace ParserUtils {

// Taken from MUME's HELP LATIN to convert from Latin-1 to US-ASCII
static const unsigned char latin1ToAscii[] = {
    /*160*/
    ' ', '!', 'c', 'L', '$',  'Y', '|', 'P', '"', 'C', 'a', '<', ',', '-', 'R', '-',
    'd', '+', '2', '3', '\'', 'u', 'P', '*', ',', '1', 'o', '>', '4', '2', '3', '?',
    'A', 'A', 'A', 'A', 'A',  'A', 'A', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
    'D', 'N', 'O', 'O', 'O',  'O', 'O', '*', 'O', 'U', 'U', 'U', 'U', 'Y', 'T', 's',
    'a', 'a', 'a', 'a', 'a',  'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'd', 'n', 'o', 'o', 'o',  'o', 'o', '/', 'o', 'u', 'u', 'u', 'u', 'y', 't', 'y'};

QString &removeAnsiMarksInPlace(QString &str)
{
    static const QRegularExpression ansi("\x1b\\[[0-9;]*[A-Za-z]");
    str.remove(ansi);
    return str;
}

QString &latinToAsciiInPlace(QString &str)
{
    for (int pos = 0; pos < str.length(); pos++) {
        auto ch = static_cast<unsigned char>(str.at(pos).toLatin1());
        if (ch > 128) {
            if (ch < 160) {
                ch = 'z';
            } else {
                ch = latin1ToAscii[ch - 160];
            }
            str[pos] = ch;
        }
    }
    return str;
}

} // namespace ParserUtils
