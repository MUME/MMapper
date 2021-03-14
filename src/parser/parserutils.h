#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <string>
#include <string_view>

#include "../global/macros.h"

class QString;

namespace ParserUtils {
QString &removeAnsiMarksInPlace(QString &str);
QString &toAsciiInPlace(QString &str);
std::string &latin1ToAsciiInPlace(std::string &str);
NODISCARD std::string latin1ToAscii(const std::string_view &sv);

} // namespace ParserUtils
