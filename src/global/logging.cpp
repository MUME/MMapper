// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "logging.h"

#include "Charset.h"
#include "LineUtils.h"
#include "TextUtils.h"

namespace mm {

AbstractDebugOStream::AbstractDebugOStream(QDebug &&os)
    : m_debug(os)
{}

AbstractDebugOStream::~AbstractDebugOStream()
{
    const auto str_utf8 = std::move(m_os_utf8).str();
    if (str_utf8.empty())
        return;

    // Try to detect accidentally passing latin1
    assert(charset::isProbablyUtf8(str_utf8));

    auto &debug = m_debug;

    // Assume Qt has a lock.
    debug.noquote();
    debug.nospace();

    // QT logging doesn't expect a newline at the end,
    // so we'll only add newlines for subsequent lines.
    bool needsNewline = false;
    foreachLine(str_utf8, [&needsNewline, &debug](std::string_view line) {
        assert(!line.empty());

        if (needsNewline) {
            debug << char_consts::C_NEWLINE;
        }
        if (!line.empty() && line.back() == char_consts::C_NEWLINE) {
            line.remove_suffix(1);
        }
        if (!line.empty() && line.back() == char_consts::C_CARRIAGE_RETURN) {
            line.remove_suffix(1);
        }

        if (!line.empty()) {
            debug << mmqt::toQByteArrayUtf8(line);
        }

        needsNewline = true;
    });
}

void AbstractDebugOStream::writeLatin1(const std::string_view sv)
{
    latin1ToUtf8(m_os_utf8, sv);
}

void AbstractDebugOStream::writeUtf8(const std::string_view sv)
{
    m_os_utf8 << sv;
}

DebugOstream::~DebugOstream() = default;
InfoOstream::~InfoOstream() = default;
WarningOstream::~WarningOstream() = default;

} // namespace mm
