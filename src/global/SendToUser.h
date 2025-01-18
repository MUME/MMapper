#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Signal2.h"

#include <QString>

namespace global {
void registerSendToUser(Signal2Lifetime &, Signal2<QString>::Function callback);
void sendToUser(const QString &str);
} // namespace global
