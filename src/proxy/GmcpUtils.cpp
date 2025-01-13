// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpUtils.h"

#include "../global/ConfigConsts.h"
#include "../global/Consts.h"
#include "../global/TextUtils.h"
#include "../global/tests.h"

#include <sstream>

#include <QString>

using namespace char_consts;

namespace GmcpUtils {

QString escapeGmcpStringData(const QString &str)
{
    using namespace char_consts;
    QString result;
    for (const QChar c : str) {
        if (c == C_DQUOTE) {
            result.append(C_BACKSLASH);
            result.append(C_DQUOTE);
        } else if (c == C_BACKSLASH) {
            result.append(C_BACKSLASH);
            result.append(C_BACKSLASH);
        } else if (c == C_NEWLINE) {
            result.append(C_BACKSLASH);
            result.append('n');
        } else if (c == C_CARRIAGE_RETURN) {
            result.append(C_BACKSLASH);
            result.append('r');
        } else if (c == C_BACKSPACE) {
            // this should probably *NEVER* be sent
            result.append(C_BACKSLASH);
            result.append('b');
        } else if (c == C_FORM_FEED) {
            // this should probably *NEVER* be sent
            result.append(C_BACKSLASH);
            result.append('f');
        } else if (c == C_TAB) {
            result.append(C_BACKSLASH);
            result.append('t');
        } else {
            result.append(c);
        }
    }
    return result;
}
} // namespace GmcpUtils

namespace test {
void testGmcpUtils()
{
    using namespace char_consts;
    QString s;
    s.append(C_DQUOTE);
    s.append(C_BACKSLASH);
    s.append(C_BACKSPACE);
    s.append(C_FORM_FEED);
    s.append(QChar(char16_t(0xFF)));
    s.append(QChar(char16_t(0x100)));
    s.append(C_CARRIAGE_RETURN);
    s.append(C_NEWLINE);
    TEST_ASSERT(s.size() == 8);

    const auto result = GmcpUtils::escapeGmcpStringData(s);
    TEST_ASSERT(result.size() == 14);

    const auto str = mmqt::toStdStringUtf8(result);
    const auto expected = "\\\""
                          "\\\\"
                          "\\b"
                          "\\f"
                          "\xC3\xBF"
                          "\xC4\x80"
                          "\\r"
                          "\\n";
    TEST_ASSERT(str == expected);
}
} // namespace test
