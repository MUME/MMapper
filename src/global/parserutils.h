#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"

#include <string>
#include <string_view>

class QString;

namespace ParserUtils {
QString &removeAnsiMarksInPlace(QString &str);

NODISCARD bool isWhitespaceNormalized(std::string_view sv);
NODISCARD std::string normalizeWhitespace(std::string str);

} // namespace ParserUtils
