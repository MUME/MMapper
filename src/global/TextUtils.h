#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Consts.h"
#include "macros.h"
#include "utils.h"

#include <cassert>
#include <cctype>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <QRegularExpression>
#include <QString>

NODISCARD static inline bool isSpace(const char c)
{
    return std::isspace(static_cast<uint8_t>(c));
}

namespace mmqt {
NODISCARD extern int findTrailingWhitespace(QStringView line);
NODISCARD extern int findTrailingWhitespace(const QString &line);
} // namespace mmqt

NODISCARD extern bool isAbbrev(std::string_view abbr, std::string_view fullText);
NODISCARD extern bool isPrintLatin1(char c);

namespace mmqt {
NODISCARD extern QString toQStringLatin1(std::string_view sv);
NODISCARD extern QString toQStringUtf8(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayLatin1(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayUtf8(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayLatin1(const QString &qs);
NODISCARD extern std::string toStdStringLatin1(const QString &qs);
NODISCARD extern std::string toStdStringUtf8(const QString &qs);
NODISCARD extern std::string_view toStdStringViewLatin1(const QByteArray &arr);
NODISCARD extern std::string_view toStdStringViewLatin1(const QByteArray &&) = delete;
} // namespace mmqt
