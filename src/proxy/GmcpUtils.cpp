// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpUtils.h"

#include <sstream>
#include <QString>

#include "../global/TextUtils.h"
#include "GmcpMessage.h"

namespace GmcpUtils {

QString escapeGmcpStringData(const QString &str)
{
    QString result;
    for (const QChar c : str) {
        if (c == '\"')
            result.append(R"(\")");
        else if (c == '\\')
            result.append(R"(\\)");
        else if (c == C_NEWLINE)
            result.append(R"(\n)");
        else if (c == C_CARRIAGE_RETURN)
            result.append(R"(\r)");
        else if (c == '\b')
            result.append(R"(\b)");
        else if (c == '\f')
            result.append(R"(\f)");
        else if (c == C_TAB)
            result.append(R"(\t)");
        else
            result.append(c);
    }
    return result;
}
} // namespace GmcpUtils
