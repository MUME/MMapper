#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"

class QString;

namespace GmcpUtils {
NODISCARD QString escapeGmcpStringData(const QString &);
} // namespace GmcpUtils
