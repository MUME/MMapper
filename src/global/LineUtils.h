#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Consts.h"
#include "utils.h"

#include <QString>

namespace mmqt {

// Callback should be:
// void(QStringView line, bool hasNewline)
template<typename Callback>
void foreachLine(const QStringView input, Callback &&callback)
{
    using char_consts::C_NEWLINE;
    const qsizetype len = input.size();
    qsizetype pos = 0;
    while (pos < len) {
        const qsizetype next = input.indexOf(C_NEWLINE, pos);
        if (next < 0)
            break;
        assert(next >= pos);
        assert(input[next] == C_NEWLINE);
        callback(input.mid(pos, next - pos), true);
        pos = next + 1;
    }
    if (pos < len)
        callback(input.mid(pos, len - pos), false);
}

template<typename Callback>
inline void foreachLine(const QString &input, Callback &&callback)
{
    foreachLine(QStringView{input}, std::forward<Callback>(callback));
}

NODISCARD extern int countLines(const QString &input);
NODISCARD extern int countLines(QStringView input);

} // namespace mmqt

// Callback = void(string_view);
template<typename Callback>
void foreachLine(const std::string_view input, Callback &&callback)
{
    using char_consts::C_NEWLINE;
    const size_t len = input.size();
    size_t pos = 0;
    while (pos < len) {
        const auto next = input.find(C_NEWLINE, pos);
        if (next == std::string_view::npos)
            break;
        assert(next >= pos);
        assert(input[next] == C_NEWLINE);
        callback(input.substr(pos, next - pos + 1));
        pos = next + 1;
    }
    if (pos < len)
        callback(input.substr(pos, len - pos));
}

NODISCARD size_t countLines(std::string_view input);
