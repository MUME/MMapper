// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "TextUtils.h"

#include <cstring>
#include <iostream>

#include <QRegularExpression>
#include <QString>

namespace mmqt {
static const QRegularExpression trailingWhitespaceRegex(R"([[:space:]]+$)");

int findTrailingWhitespace(const QStringView line)
{
    auto m = trailingWhitespaceRegex.match(line);
    if (!m.hasMatch()) {
        return -1;
    }
    return m.capturedStart();
}

int findTrailingWhitespace(const QString &line)
{
    return findTrailingWhitespace(QStringView{line});
}
} // namespace mmqt

bool isAbbrev(const std::string_view abbr, const std::string_view fullText)
{
    return !abbr.empty()                               //
           && abbr.size() <= fullText.size()           //
           && abbr == fullText.substr(0, abbr.size()); //
}

namespace mmqt {

QString toQStringLatin1(const std::string_view sv)
{
    return QString::fromLatin1(sv.data(), static_cast<int>(sv.size()));
}

QString toQStringUtf8(const std::string_view sv)
{
    return QString::fromUtf8(sv.data(), static_cast<int>(sv.size()));
}

QByteArray toQByteArrayLatin1(const std::string_view sv)
{
    return QByteArray(sv.data(), static_cast<int>(sv.size()));
}

QByteArray toQByteArrayUtf8(const std::string_view sv)
{
    return toQStringUtf8(sv).toUtf8();
}

std::string toStdStringLatin1(const QString &qs)
{
    return qs.toLatin1().toStdString();
}

std::string toStdStringUtf8(const QString &qs)
{
    return qs.toUtf8().toStdString();
}

std::string_view toStdStringViewLatin1(const QByteArray &arr)
{
    return std::string_view{arr.data(), static_cast<size_t>(arr.size())};
}
} // namespace mmqt
