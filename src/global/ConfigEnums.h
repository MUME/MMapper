#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "macros.h"

enum class NODISCARD AutoLoggerEnum { KeepForever, DeleteDays, DeleteSize };
enum class NODISCARD CharacterEncodingEnum { LATIN1, UTF8, ASCII };
enum class NODISCARD EnvironmentEnum { Unknown, Env32Bit, Env64Bit };
enum class NODISCARD MapModeEnum { PLAY, MAP, OFFLINE };
// How the game is played: the Client Panel asks each time, connects its
// built-in client right away, or waits for an external client on the proxy
// port. The web build has only the built-in client.
enum class NODISCARD GameClientEnum { ASK, BUILT_IN, EXTERNAL };
enum class NODISCARD ThemeEnum { System, Dark, Light };
enum class NODISCARD PackageEnum { Source, Deb, Dmg, Nsis, AppImage, AppX, Flatpak, Snap, Wasm };
enum class NODISCARD PlatformEnum { Unknown, Windows, Mac, Linux, Wasm };
