#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include <QObject>
#include <QtGlobal>

class ProgressCounter final : public QObject
{
    Q_OBJECT

private:
    quint32 m_totalSteps = 0u, m_steps = 0u, m_percentage = 0u;

public:
    ProgressCounter() = default;
    explicit ProgressCounter(QObject *parent);
    ~ProgressCounter() final;

public:
    void step(quint32 steps = 1u);
    void increaseTotalStepsBy(quint32 steps);
    void reset();

signals:
    void sig_onPercentageChanged(quint32);
};
