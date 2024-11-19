// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpUtils.h"

#include "../global/Consts.h"
#include "../global/TextUtils.h"
#include "GmcpMessage.h"

#include <sstream>

#include <QString>

using namespace char_consts;

namespace GmcpUtils {

QString escapeGmcpStringData(const QString &str)
{
    QString result;
    for (const QChar c : str) {
        if (c == C_DQUOTE)
            result.append(R"(\")");
        else if (c == C_BACKSLASH)
            result.append(R"(\\)");
        else if (c == C_NEWLINE)
            result.append(R"(\n)");
        else if (c == C_CARRIAGE_RETURN)
            result.append(R"(\r)");
        else if (c == C_BACKSPACE)
            result.append(R"(\b)");
        else if (c == C_FORM_FEED)
            result.append(R"(\f)");
        else if (c == C_TAB)
            result.append(R"(\t)");
        else
            result.append(c);
    }
    return result;
}

namespace { // anonymous
namespace test {
static int testing()
{
    QString s;
    s.append(C_DQUOTE);
    s.append(C_BACKSLASH);
    s.append(C_BACKSPACE);
    s.append(C_FORM_FEED);
    s.append(QChar(char16_t(0xff)));
    s.append(QChar(char16_t(0x100)));
    s.append(C_CARRIAGE_RETURN);
    s.append(C_NEWLINE);
    assert(s.size() == 8);

    const auto result = escapeGmcpStringData(s);
    assert(result.size() == 14);

    const auto str = mmqt::toStdStringUtf8(result);
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
} // namespace
} // namespace GmcpUtils
