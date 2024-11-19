// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "progresscounter.h"

#include <QObject>

ProgressCanceledException::ProgressCanceledException()
    : std::runtime_error("ProgressCanceledException")
{}
ProgressCanceledException::~ProgressCanceledException() = default;

void ProgressCounter::setNewTask(const ProgressMsg &currentTask, const size_t newTotalSteps)
{
    checkCancel();

    std::lock_guard<std::mutex> lock{m_mutex};
    m_status.msg = currentTask;
    m_status.reset(newTotalSteps);
}

void ProgressCounter::setCurrentTask(const ProgressMsg &currentTask)
{
    checkCancel();

    std::lock_guard<std::mutex> lock{m_mutex};
    m_status.msg = currentTask;
}

void ProgressCounter::increaseTotalStepsBy(const size_t steps)
{
    checkCancel();

    std::lock_guard<std::mutex> lock{m_mutex};
    m_status.expected += steps;
}

void ProgressCounter::step(const size_t steps)
{
    checkCancel();

    std::lock_guard<std::mutex> lock{m_mutex};
    m_status.seen += steps;
}

ProgressMsg ProgressCounter::getCurrentTask() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_status.msg;
}

size_t ProgressCounter::getPercentage() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_status.percent();
}

ProgressCounter::Status ProgressCounter::getStatus() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_status;
}

void ProgressCounter::reset()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_status.reset();
}

bool tags::TagProgressMsg::isValid(std::string_view /*sv*/)
{
    return true;
}
