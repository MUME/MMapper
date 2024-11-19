#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "utils.h"

#include <cassert>

#include <QString>

// Callback = void(string_view);
// callback is either a span excluding c, or a span of contiguous c's.
template<typename Callback>
void foreachChar(const std::string_view input, const char c, Callback &&callback)
{
    std::string_view sv = input;
    while (!sv.empty()) {
        if (const auto next = sv.find(c); next == std::string_view::npos) {
            callback(sv);
            break;
        } else if (next > 0) {
            callback(sv.substr(0, next));
            sv.remove_prefix(next);
        }

        assert(!sv.empty() && sv.front() == c);
        if (const auto span = sv.find_first_not_of(c); span == std::string_view::npos) {
            callback(sv);
            break;
        } else {
            callback(sv.substr(0, span));
            sv.remove_prefix(span);
        }
    }
}

namespace mmqt {

// Callback is void(int pos)
template<typename Callback>
void foreachChar(const QStringView input, const char c, Callback &&callback)
{
    const qsizetype len = input.size();
    qsizetype pos = 0;
    while (pos < len) {
        const qsizetype next = input.indexOf(c, pos);
        if (next < 0)
            break;
        assert(next >= pos);
        assert(input[next] == c);
        callback(next);
        pos = next + 1;
    }
}

template<typename Callback>
inline void foreachChar(const QString &input, const char c, Callback &&callback)
{
    foreachChar(QStringView{input}, c, std::forward<Callback>(callback));
}

} // namespace mmqt
