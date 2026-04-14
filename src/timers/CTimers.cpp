// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "CTimers.h"

#include "../global/thread_utils.h"
#include "../global/utils.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

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
    if (m_expired) {
        return m_duration;
    }
    return nowMs() - m_start;
}

int64_t TTimer::remainingMs() const
{
    if (m_expired) {
        return 0;
    }
    return std::max<int64_t>(0, m_duration - elapsedMs());
}

CTimers::CTimers(QObject *const parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);

    connect(&m_timer, &QTimer::timeout, this, &CTimers::slot_finishCountdownTimer);
}

void CTimers::addTimer(std::string name, std::string desc)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    std::ignore = removeTimer(name);

    // Insert before first expired timer
    auto it = std::find_if(m_allTimers.begin(), m_allTimers.end(), [](const TTimer &t) {
        return t.isExpired();
    });
    m_allTimers.emplace(it, std::move(name), std::move(desc));

    emit sig_timerAdded();
}

bool CTimers::removeCountdown(const std::string &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    const bool removed = utils::listRemoveIf(m_allTimers, [&name](const TTimer &timer) -> bool {
        return timer.isCountdown() && timer.getName() == name;
    });
    if (removed) {
        restartCountdownTimer();
        emit sig_timerRemoved();
    }
    return removed;
}

bool CTimers::removeTimer(const std::string &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    const bool removed = utils::listRemoveIf(m_allTimers, [&name](const TTimer &timer) -> bool {
        return !timer.isCountdown() && timer.getName() == name;
    });
    if (removed) {
        emit sig_timerRemoved();
    }
    return removed;
}

void CTimers::addCountdown(std::string name, std::string desc, int64_t timeMs)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    std::ignore = removeCountdown(name);

    // Insert before first expired timer
    auto it = std::find_if(m_allTimers.begin(), m_allTimers.end(), [](const TTimer &t) {
        return t.isExpired();
    });
    m_allTimers.emplace(it, std::move(name), std::move(desc), timeMs);

    restartCountdownTimer();
    emit sig_timerAdded();
}

void CTimers::stopTimer(const std::string &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    for (auto it = m_allTimers.begin(); it != m_allTimers.end(); ++it) {
        if (!it->isCountdown() && it->getName() == name) {
            it->setExpired(true);
            // Move to end
            m_allTimers.splice(m_allTimers.end(), m_allTimers, it);
            emit sig_timersUpdated();
            break;
        }
    }
}

void CTimers::stopCountdown(const std::string &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    for (auto it = m_allTimers.begin(); it != m_allTimers.end(); ++it) {
        if (it->isCountdown() && it->getName() == name) {
            it->setExpired(true);
            // Move to end
            m_allTimers.splice(m_allTimers.end(), m_allTimers, it);
            restartCountdownTimer();
            emit sig_timersUpdated();
            break;
        }
    }
}

void CTimers::resetTimer(const std::string &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    for (auto it = m_allTimers.begin(); it != m_allTimers.end(); ++it) {
        if (!it->isCountdown() && it->getName() == name) {
            *it = TTimer(it->getName(), it->getDescription());
            // Move before first expired timer
            auto itExp = std::find_if(m_allTimers.begin(), m_allTimers.end(), [](const TTimer &t) {
                return t.isExpired();
            });
            m_allTimers.splice(itExp, m_allTimers, it);
            emit sig_timersUpdated();
            break;
        }
    }
}

void CTimers::resetCountdown(const std::string &name)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    for (auto it = m_allTimers.begin(); it != m_allTimers.end(); ++it) {
        if (it->isCountdown() && it->getName() == name) {
            *it = TTimer(it->getName(), it->getDescription(), it->durationMs());
            // Move before first expired timer
            auto itExp = std::find_if(m_allTimers.begin(), m_allTimers.end(), [](const TTimer &t) {
                return t.isExpired();
            });
            m_allTimers.splice(itExp, m_allTimers, it);
            restartCountdownTimer();
            emit sig_timersUpdated();
            break;
        }
    }
}

void CTimers::clearExpired()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    const bool removed = utils::erase_if(m_allTimers, [](const TTimer &t) { return t.isExpired(); })
                         > 0;
    if (removed) {
        emit sig_timerRemoved();
    }
}

void CTimers::slot_finishCountdownTimer()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    // See if we need to restart the timer
    {
        std::vector<std::string> updates;
        bool anyFinished = false;

        auto it = m_allTimers.begin();
        while (it != m_allTimers.end()) {
            if (!it->isCountdown() || it->isExpired() || it->remainingMs() > 0) {
                ++it;
                continue;
            }

            it->setExpired(true);
            anyFinished = true;

            std::ostringstream ostr;
            ostr << "Countdown timer " << it->getName();
            if (!it->getDescription().empty()) {
                ostr << " <" << it->getDescription() << ">";
            }
            ostr << " finished.\n";
            updates.emplace_back(std::move(ostr).str());

            // Move to end
            auto toMove = it;
            ++it;
            m_allTimers.splice(m_allTimers.end(), m_allTimers, toMove);
        }

        for (const std::string &s : updates) {
            emit sig_sendTimersUpdateToUser(s);
        }
        if (anyFinished) {
            emit sig_timersUpdated();
        }
    }

    restartCountdownTimer();
}

void CTimers::restartCountdownTimer()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    m_timer.stop();

    static auto get_diff = [](const TTimer &t) -> int64_t { return t.remainingMs(); };

    std::vector<TTimer> activeCountdowns;
    for (const auto &t : m_allTimers) {
        if (t.isCountdown() && !t.isExpired()) {
            activeCountdowns.push_back(t);
        }
    }

    if (activeCountdowns.empty()) {
        return;
    }

    const int64_t next = *utils::find_min_computed(activeCountdowns, get_diff);
    // Restart the timer
    if (next > 0) {
        m_timer.start(static_cast<int>(next));
    } else {
        // Should not really happen as we just set expired ones to true,
        // but let's be safe and trigger finish again if there's an immediate one.
        QTimer::singleShot(0, this, &CTimers::slot_finishCountdownTimer);
    }
}

std::string CTimers::getTimers() const
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    bool any = false;
    for (const auto &t : m_allTimers) {
        if (!t.isCountdown()) {
            any = true;
            break;
        }
    }

    if (!any) {
        return "";
    }

    std::ostringstream ostr;
    ostr << "Timers:" << std::endl;
    for (const auto &timer : m_allTimers) {
        if (timer.isCountdown())
            continue;
        const auto elapsed = msToMinSec(timer.elapsedMs());
        ostr << "- " << timer.getName();
        if (!timer.getDescription().empty()) {
            ostr << " <" << timer.getDescription() << ">";
        }
        ostr << " (up for - " << elapsed << ")" << std::endl;
    }
    return ostr.str();
}

std::string CTimers::getCountdowns() const
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    bool any = false;
    for (const auto &t : m_allTimers) {
        if (t.isCountdown()) {
            any = true;
            break;
        }
    }

    if (!any) {
        return "";
    }

    std::ostringstream ostr;
    ostr << "Countdowns:" << std::endl;
    for (const auto &countdown : m_allTimers) {
        if (!countdown.isCountdown())
            continue;
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

std::string CTimers::getStatCommandEntry() const
{
    return getCountdowns() + getTimers();
}

void CTimers::clear()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    m_allTimers.clear();
    m_timer.stop();
    emit sig_timerRemoved();
}

void CTimers::moveTimer(int from, int to)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    if (from < 0 || from >= static_cast<int>(m_allTimers.size()))
        return;
    if (to < 0 || to > static_cast<int>(m_allTimers.size()))
        return;

    auto itFrom = m_allTimers.begin();
    std::advance(itFrom, from);

    auto itTo = m_allTimers.begin();
    std::advance(itTo, to);

    m_allTimers.splice(itTo, m_allTimers, itFrom);

    emit sig_timersUpdated();
}
