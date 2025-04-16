#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <memory>

#include <QColor>

class ColorGeneratorImpl;

class ColorGenerator final
{
public:
    explicit ColorGenerator(QColor initialColor);
    ~ColorGenerator();

    DEFAULT_MOVES_DELETE_COPIES(ColorGenerator);

    NODISCARD QColor getNextColor();
    void releaseColor(QColor color);

private:
    std::unique_ptr<ColorGeneratorImpl> m_impl;
};
