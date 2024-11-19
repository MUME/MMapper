// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "CTimers.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <list>
#include <optional>
#include <sstream>
#include <string>

#include <QMutexLocker>

NODISCARD static inline int64_t nowMs()
{
    using namespace std::chrono;
    const auto now = steady_clock::now();
    return time_point_cast<milliseconds>(now).time_since_epoch().count();
}

TTimer::TTimer(const std::string &name, const std::string &desc, int64_t durationMs)
    : m_name{name}
    , m_desc{desc}
    , m_start{nowMs()}
    , m_duration{durationMs}
{}

TTimer::TTimer(const std::string &name, const std::string &desc)
    : TTimer(name, desc, 0)
{}

int64_t TTimer::elapsedMs() const
{
    return nowMs() - m_start;
}

CTimers::CTimers(QObject *const parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);

    connect(&m_timer, &QTimer::timeout, this, &CTimers::slot_finishCountdownTimer);
}

NODISCARD static std::string msToMinSec(const int64_t ms)
{
    const auto hour = ms / 1000 / 60 / 60;
    const auto min = ms / 1000 / 60 % 60;
    const auto sec = ms / 1000 % 60;

    std::ostringstream ostr;
    ostr << std::setfill('0');
    if (hour) {
        ostr << hour << ":" << std::setw(2);
    }
    ostr << min << ":" << std::setw(2) << sec;
    return ostr.str();
}

void CTimers::addTimer(const std::string &name, const std::string &desc)
{
    QMutexLocker locker(&m_lock);

    TTimer timer(name, desc);
    m_timers.push_back(std::move(timer));
}

bool CTimers::removeCountdown(const std::string &name)
{
    QMutexLocker locker(&m_lock);

    for (std::list<TTimer>::iterator it = m_countdowns.begin(); it != m_countdowns.end(); ++it) {
        if (it->getName() == name) {
            m_countdowns.erase(it);
            return true;
        }
    }

    return false;
}

bool CTimers::removeTimer(const std::string &name)
{
    QMutexLocker locker(&m_lock);

    for (std::list<TTimer>::iterator it = m_timers.begin(); it != m_timers.end(); ++it) {
        if (it->getName() == name) {
            m_timers.erase(it);
            return true;
        }
    }

    return false;
}

void CTimers::addCountdown(const std::string &name, const std::string &desc, int64_t timeMs)
{
    QMutexLocker locker(&m_lock);

    TTimer countdown(name, desc, timeMs);
    m_countdowns.push_back(std::move(countdown));

    // See if we need to restart the timer
    if (m_timer.isActive()) {
        std::chrono::milliseconds left = m_timer.remainingTimeAsDuration();
        if (timeMs < left.count()) {
            m_timer.stop();
            m_timer.start(static_cast<int>(timeMs));
        }
        return;
    }

    // Start the timer
    m_timer.start(static_cast<int>(timeMs));
}

void CTimers::slot_finishCountdownTimer()
{
    QMutexLocker locker(&m_lock);

    std::optional<int64_t> next;

    // See if we need to restart the timer
    std::list<TTimer>::iterator it = m_countdowns.begin();
    while (it != m_countdowns.end()) {
        const auto diff = it->durationMs() - it->elapsedMs();
        if (diff <= 0) {
            std::ostringstream ostr;
            ostr << "Countdown timer " << it->getName();
            if (!it->getDescription().empty())
                ostr << " <" << it->getDescription() << ">";
            ostr << " finished." << std::endl;
            emit sig_sendTimersUpdateToUser(ostr.str());
            it = m_countdowns.erase(it);
        } else {
            if (next)
                next = std::min(diff, next.value());
            else
                next = diff;
            ++it;
        }
    }

    if (m_countdowns.empty())
        return;

    if (m_timer.isActive())
        m_timer.stop();

    // Restart the timer
    if (next.value_or(0) > 0)
        m_timer.start(static_cast<int>(next.value()));
}

std::string CTimers::getTimers()
{
    if (m_timers.empty())
        return "";

    std::ostringstream ostr;
    ostr << "Timers:" << std::endl;
    for (const auto &timer : m_timers) {
        const auto elapsed = msToMinSec(timer.elapsedMs());
        ostr << "- " << timer.getName();
        if (!timer.getDescription().empty())
            ostr << " <" << timer.getDescription() << ">";
        ostr << " (up for - " << elapsed << ")" << std::endl;
    }
    return ostr.str();
}

std::string CTimers::getCountdowns()
{
    if (m_countdowns.empty())
        return "";

    std::ostringstream ostr;
    ostr << "Countdowns:" << std::endl;
    for (const auto &countdown : m_countdowns) {
        const auto elapsed = msToMinSec(countdown.elapsedMs());
        const auto left = msToMinSec(countdown.durationMs() - countdown.elapsedMs());
        ostr << "- " << countdown.getName();
        if (!countdown.getDescription().empty())
            ostr << " <" << countdown.getDescription() << ">";
        ostr << " (up for - " << elapsed << ", left - " << left << ")" << std::endl;
    }
    return ostr.str();
}

std::string CTimers::getStatCommandEntry()
{
    QMutexLocker locker(&m_lock);

    return getCountdowns() + getTimers();
}

void CTimers::clear()
{
    QMutexLocker locker(&m_lock);

    m_countdowns.clear();
    m_timers.clear();
}
