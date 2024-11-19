// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "progresscounter.h"

#include <QObject>

ProgressCounter::~ProgressCounter() = default;

ProgressCounter::ProgressCounter(QObject *parent)
    : QObject(parent)
{}

void ProgressCounter::increaseTotalStepsBy(size_t steps)
{
    m_totalSteps += steps;
    step(0u);
}

void ProgressCounter::step(const size_t steps)
{
    m_steps += steps;
    const size_t percentage = (m_totalSteps == 0u) ? 0u : (100u * m_steps / m_totalSteps);
    if (percentage != m_percentage) {
        m_percentage = percentage;
        emit sig_onPercentageChanged(percentage);
    }
}

void ProgressCounter::reset()
{
    m_totalSteps = m_steps = m_percentage = 0u;
}
