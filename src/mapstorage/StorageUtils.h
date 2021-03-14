#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

class QByteArray;

namespace StorageUtils {
NODISCARD QByteArray inflate(QByteArray &);
} // namespace StorageUtils
