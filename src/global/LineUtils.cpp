// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "LineUtils.h"

namespace mmqt {

int countLines(const QStringView input)
{
    int count = 0;
    foreachLine(input, [&count](const QStringView, bool) { ++count; });
    return count;
}

int countLines(const QString &input)
{
    return countLines(QStringView{input});
}

} // namespace mmqt

size_t countLines(const std::string_view input)
{
    size_t count = 0;
    foreachLine(input, [&count](const std::string_view) { ++count; });
    return count;
}
