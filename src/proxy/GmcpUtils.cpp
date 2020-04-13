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
    std::ostringstream oss;
    for (char c : str.toUtf8()) {
        if (c == static_cast<char>(0xFF)) // 0xFF is not allowed as a valid character
            continue;
        else if (c == '"') // double-quote
            oss << R"(\")";
        else if (c == '\\') // backslash
            oss << R"(\\)";
        else if (c == C_NEWLINE) // new line
            oss << R"(\n)";
        else if (c == C_CARRIAGE_RETURN) // carriage-return
            oss << R"(\r)";
        else if (c == '\b') // backspace
            oss << R"(\b)";
        else if (c == '\f') // form-feed
            oss << R"(\f)";
        else if (c == C_TAB) // tab
            oss << R"(\t)";
        else
            oss << c;
    }
    return ::toQStringUtf8(oss.str());
}
} // namespace GmcpUtils
