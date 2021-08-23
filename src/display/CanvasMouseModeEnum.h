#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"

enum class NODISCARD CanvasMouseModeEnum {
    NONE,
    MOVE,
    RAYPICK_ROOMS,
    SELECT_ROOMS,
    SELECT_CONNECTIONS,
    CREATE_ROOMS,
    CREATE_CONNECTIONS,
    CREATE_ONEWAY_CONNECTIONS,
    SELECT_INFOMARKS,
    CREATE_INFOMARKS
};
