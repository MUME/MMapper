// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ColorGenerator.h"

#include <cmath>

#include <QColor>
#include <queue>

struct ColorGeneratorImpl final
{
    static const constexpr double GOLDEN_ANGLE = 137.508; // in degrees
    double hue;
    std::queue<int> prevHues;

    explicit ColorGeneratorImpl(QColor initialColor)
        : hue{static_cast<double>(initialColor.hue())}
    {}

    QColor getNextColor()
    {
        QColor next;
        if (!prevHues.empty()) {
            next = QColor::fromHsl(prevHues.front(), 255, 127);
            prevHues.pop();
        } else {
            hue = fmod(hue + GOLDEN_ANGLE, 360.0);
            next = QColor::fromHsl(static_cast<int>(hue + 0.5), 255, 127);
        }
        return next;
    }

    void releaseColor(QColor color)
    {
        if (color.isValid()) {
            prevHues.emplace(color.hue());
        }
    }
};

ColorGenerator::ColorGenerator(QColor initialColor)
    : m_impl{std::make_unique<ColorGeneratorImpl>(initialColor)}
{}

ColorGenerator::~ColorGenerator() = default;

QColor ColorGenerator::getNextColor()
{
    return m_impl->getNextColor();
}

void ColorGenerator::releaseColor(QColor color)
{
    m_impl->releaseColor(color);
}
