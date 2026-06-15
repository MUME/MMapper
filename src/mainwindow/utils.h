#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

class QObject;
class MapFrontend;
class MapWindow;

class NODISCARD CanvasDisabler final
{
private:
    MapWindow &m_window;

public:
    explicit CanvasDisabler(MapWindow &window);
    ~CanvasDisabler();
    DELETE_CTORS_AND_ASSIGN_OPS(CanvasDisabler);
};

class NODISCARD MapFrontendBlocker final
{
private:
    MapFrontend &m_data;

public:
    MapFrontendBlocker() = delete;
    explicit MapFrontendBlocker(MapFrontend &data);
    ~MapFrontendBlocker();
    DELETE_CTORS_AND_ASSIGN_OPS(MapFrontendBlocker);
};
