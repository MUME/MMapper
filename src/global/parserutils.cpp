// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "parserutils.h"

#include "Charset.h"
#include "Consts.h"
#include "TextUtils.h"

#include <QRegularExpression>

namespace ParserUtils {

QString &removeAnsiMarksInPlace(QString &str)
{
    static const QRegularExpression ansi("\x1B\\[[0-9;:]*[A-Za-z]");
    str.remove(ansi);
    return str;
}

NODISCARD bool isWhitespaceNormalized(const std::string_view sv)
{
    bool last_was_space = false;
    for (char c : sv) {
        if (c == char_consts::C_SPACE) {
            if (last_was_space) {
                return false;
            } else {
                last_was_space = true;
            }
        } else if (isSpace(c)) {
            return false;
        } else {
            last_was_space = false;
        }
    }

    return true;
}

std::string normalizeWhitespace(std::string str)
{
    if (!isWhitespaceNormalized(str)) {
        const size_t len = str.size();
        bool last_was_space = false;
        size_t out = 0;
        for (size_t in = 0; in < len; ++in) {
            const char c = str[in];
            if (isSpace(c)) {
                if (!last_was_space) {
                    last_was_space = true;
                    str[out++] = char_consts::C_SPACE;
                }
            } else {
                last_was_space = false;
                str[out++] = c;
            }
        }
        str.resize(out);
        assert(isWhitespaceNormalized(str));
    }
    return str;
}

} // namespace ParserUtils
