#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

class QObject;
class MapCanvas;
class MapFrontend;

class NODISCARD CanvasDisabler final
{
private:
    MapCanvas &canvas;

public:
    explicit CanvasDisabler(MapCanvas &in_canvas);
    ~CanvasDisabler();
    DELETE_CTORS_AND_ASSIGN_OPS(CanvasDisabler);
};

class NODISCARD MapFrontendBlocker final
{
public:
    explicit MapFrontendBlocker(MapFrontend &in_data);
    ~MapFrontendBlocker();

public:
    MapFrontendBlocker() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(MapFrontendBlocker);

private:
    MapFrontend &data;
};
