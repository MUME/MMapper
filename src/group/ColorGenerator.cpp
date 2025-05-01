// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ColorGenerator.h"

#include <cmath>
#include <deque>

#include <QColor>

struct ColorGeneratorImpl final
{
    static const constexpr double GOLDEN_ANGLE = 137.508; // in degrees
    double hue = 255;
    std::deque<int> prevHues;

    QColor getNextColor()
    {
        QColor next;
        if (!prevHues.empty()) {
            next = QColor::fromHsl(prevHues.front(), 255, 127);
            prevHues.pop_front();
        } else {
            hue = fmod(hue + GOLDEN_ANGLE, 360.0);
            next = QColor::fromHsl(static_cast<int>(hue + 0.5), 255, 127);
        }
        return next;
    }

    void releaseColor(QColor color)
    {
        if (color.isValid()) {
            prevHues.emplace_back(color.hue());
        }
    }

    void init(QColor color)
    {
        prevHues.clear();
        hue = static_cast<double>(color.hue());
    }
};

ColorGenerator::ColorGenerator()
    : m_impl{std::make_unique<ColorGeneratorImpl>()}
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

void ColorGenerator::init(QColor color)
{
    m_impl->init(color);
}
