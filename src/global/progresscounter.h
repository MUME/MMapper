#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include <cstddef>

#include <QObject>
#include <QtGlobal>

class ProgressCounter final : public QObject
{
    Q_OBJECT

private:
    size_t m_totalSteps = 0u, m_steps = 0u, m_percentage = 0u;

public:
    ProgressCounter() = default;
    explicit ProgressCounter(QObject *parent);
    ~ProgressCounter() final;

public:
    void step(size_t steps = 1u);
    void increaseTotalStepsBy(size_t steps);
    void reset();

signals:
    void sig_onPercentageChanged(size_t);
};
