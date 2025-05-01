#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <memory>

#include <QColor>

struct ColorGeneratorImpl;

class ColorGenerator final
{
private:
    std::unique_ptr<ColorGeneratorImpl> m_impl;

public:
    explicit ColorGenerator();
    ~ColorGenerator();

    DEFAULT_MOVES_DELETE_COPIES(ColorGenerator);

public:
    void init(QColor color);

public:
    NODISCARD QColor getNextColor();
    void releaseColor(QColor color);
};
