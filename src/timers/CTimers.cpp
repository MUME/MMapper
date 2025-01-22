// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "CTimers.h"

#include "../global/utils.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <QMutexLocker>

namespace { // anonymous

NODISCARD int64_t nowMs()
{
    using namespace std::chrono;
    const auto now = steady_clock::now();
    return time_point_cast<milliseconds>(now).time_since_epoch().count();
}

NODISCARD std::string msToMinSec(const int64_t ms)
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

} // namespace

TTimer::TTimer(std::string name, std::string desc, int64_t durationMs)
    : m_name{std::move(name)}
    , m_desc{std::move(desc)}
    , m_start{nowMs()}
    , m_duration{durationMs}
{}

TTimer::TTimer(std::string name, std::string desc)
    : TTimer{std::move(name), std::move(desc), 0}
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

void CTimers::addTimer(std::string name, std::string desc)
{
    QMutexLocker locker(&m_lock);
    m_timers.emplace_back(std::move(name), std::move(desc));
}

bool CTimers::removeCountdown(const std::string &name)
{
    QMutexLocker locker(&m_lock);
    return utils::listRemoveIf(m_countdowns, [&name](const TTimer &timer) -> bool {
        return timer.getName() == name;
    });
}

bool CTimers::removeTimer(const std::string &name)
{
    QMutexLocker locker(&m_lock);
    return utils::listRemoveIf(m_timers, [&name](const TTimer &timer) -> bool {
        return timer.getName() == name;
    });
}

void CTimers::addCountdown(std::string name, std::string desc, int64_t timeMs)
{
    QMutexLocker locker(&m_lock);
    m_countdowns.emplace_back(std::move(name), std::move(desc), timeMs);

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

    static auto get_diff = [](const TTimer &t) -> int64_t { return t.durationMs() - t.elapsedMs(); };

    // See if we need to restart the timer
    {
        std::vector<std::string> updates;
        utils::erase_if(m_countdowns, [&updates](TTimer &t) -> bool {
            if (get_diff(t) > 0) {
                return false;
            }
            std::ostringstream ostr;
            ostr << "Countdown timer " << t.getName();
            if (!t.getDescription().empty()) {
                ostr << " <" << t.getDescription() << ">";
            }
            // why does this include a newline?
            ostr << " finished.\n";
            updates.emplace_back(std::move(ostr).str());
            return true;
        });
        for (const std::string &s : updates) {
            emit sig_sendTimersUpdateToUser(s);
        }
    }

    // Why do we return before stopping the timer?
    if (m_countdowns.empty()) {
        return;
    }

    // Why do we stop unconditionally and then restart?
    if (m_timer.isActive()) {
        m_timer.stop();
    }

    const int64_t next = *utils::find_min_computed(m_countdowns, get_diff);
    // Restart the timer
    if (next > 0) {
        m_timer.start(static_cast<int>(next));
    }
}

std::string CTimers::getTimers()
{
    QMutexLocker locker(&m_lock);

    if (m_timers.empty()) {
        return "";
    }

    std::ostringstream ostr;
    ostr << "Timers:" << std::endl;
    for (const auto &timer : m_timers) {
        const auto elapsed = msToMinSec(timer.elapsedMs());
        ostr << "- " << timer.getName();
        if (!timer.getDescription().empty()) {
            ostr << " <" << timer.getDescription() << ">";
        }
        ostr << " (up for - " << elapsed << ")" << std::endl;
    }
    return ostr.str();
}

std::string CTimers::getCountdowns()
{
    QMutexLocker locker(&m_lock);

    if (m_countdowns.empty()) {
        return "";
    }

    std::ostringstream ostr;
    ostr << "Countdowns:" << std::endl;
    for (const auto &countdown : m_countdowns) {
        const auto elapsed = msToMinSec(countdown.elapsedMs());
        const auto left = msToMinSec(countdown.durationMs() - countdown.elapsedMs());
        ostr << "- " << countdown.getName();
        if (!countdown.getDescription().empty()) {
            ostr << " <" << countdown.getDescription() << ">";
        }
        ostr << " (up for - " << elapsed << ", left - " << left << ")" << std::endl;
    }
    return ostr.str();
}

std::string CTimers::getStatCommandEntry()
{
    return getCountdowns() + getTimers();
}

void CTimers::clear()
{
    QMutexLocker locker(&m_lock);

    m_countdowns.clear();
    m_timers.clear();
}
