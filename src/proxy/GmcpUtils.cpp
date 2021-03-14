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

namespace test {
static int testing()
{
    QString s;
    s.append('\"');
    s.append('\\');
    s.append('\b');
    s.append('\f');
    s.append(QChar(char16_t(0xff)));
    s.append(QChar(char16_t(0x100)));
    s.append(C_CARRIAGE_RETURN);
    s.append(C_NEWLINE);
    assert(s.size() == 8);

    const auto result = escapeGmcpStringData(s);
    assert(result.size() == 14);

    const auto str = ::toStdStringUtf8(result);
    assert(str
           == "\\\""
              "\\\\"
              "\\b"
              "\\f"
              "\xc3\xbf"
              "\xc4\x80"
              "\\r"
              "\\n");
    return 42;
}
MAYBE_UNUSED static int test = testing();
} // namespace test

} // namespace GmcpUtils
