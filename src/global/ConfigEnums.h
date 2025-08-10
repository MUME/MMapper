#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "macros.h"

enum class NODISCARD AutoLoggerEnum { KeepForever, DeleteDays, DeleteSize };
enum class NODISCARD CharacterEncodingEnum { LATIN1, UTF8, ASCII };
enum class NODISCARD EnvironmentEnum { Unknown, Env32Bit, Env64Bit };
enum class NODISCARD MapModeEnum { PLAY, MAP, OFFLINE };
enum class NODISCARD PlatformEnum { Unknown, Windows, Mac, Linux, Wasm };
