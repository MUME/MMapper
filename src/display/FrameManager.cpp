// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "FrameManager.h"

#include "../configuration/configuration.h"

#include <algorithm>

#include <QOpenGLWindow>

FrameManager::FrameManager(QOpenGLWindow &window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
    updateMinFrameTime();
    setConfig().canvas.advanced.maximumFps.registerChangeCallback(m_configLifetime, [this]() {
        updateMinFrameTime();
    });

    m_heartbeatTimer.setSingleShot(true);
    m_heartbeatTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &FrameManager::onHeartbeat);

    requestFrame();
}

void FrameManager::registerCallback(const Signal2Lifetime &lifetime, AnimationCallback callback)
{
    m_callbacks.push_back({lifetime.getObj(), std::move(callback)});
}

bool FrameManager::needsHeartbeat() const
{
    if (m_animating || m_dirty) {
        return true;
    }

    bool anyActive = false;
    bool needsCleanup = false;

    for (const auto &entry : m_callbacks) {
        if (auto shared = entry.lifetime.lock()) {
            if (entry.callback() == AnimationStatus::Continue) {
                anyActive = true;
                // Defer thorough cleanup to when heartbeat is truly idle.
                break;
            }
        } else {
            needsCleanup = true;
        }
    }

    if (needsCleanup) {
        cleanupExpiredCallbacks();
    }

    return anyActive;
}

void FrameManager::cleanupExpiredCallbacks() const
{
    auto it = std::remove_if(m_callbacks.begin(), m_callbacks.end(), [](const Entry &e) {
        return e.lifetime.expired();
    });
    m_callbacks.erase(it, m_callbacks.end());
}

void FrameManager::requestUpdate()
{
    m_dirty = true;
    requestFrame();
}

std::optional<FrameManager::Frame> FrameManager::beginFrame()
{
    const auto now = std::chrono::steady_clock::now();
    const bool hasLastUpdate = (m_lastUpdateTime.time_since_epoch().count() != 0);

    // Throttle: check if enough time has passed since the start of the last successful frame.
    if (hasLastUpdate) {
        const auto elapsed = now - m_lastUpdateTime;
        if (elapsed + getJitterTolerance() < m_minFrameTime) {
            return std::nullopt;
        }
    }

    // Deduplication: skip frame if nothing has changed and we aren't animating.
    if (!m_dirty && !needsHeartbeat()) {
        return std::nullopt;
    }
    m_dirty = false;

    m_lastUpdateTime = now;
    return Frame(*this);
}

void FrameManager::setAnimating(bool value)
{
    if (m_animating == value) {
        return;
    }

    m_animating = value;

    if (m_animating && !m_heartbeatTimer.isActive()) {
        onHeartbeat();
    }
}

void FrameManager::onHeartbeat()
{
    if (!needsHeartbeat()) {
        m_animating = false;
        m_heartbeatTimer.stop();
        return;
    }

    m_window.update();

    if (needsHeartbeat()) {
        // We always schedule a fallback heartbeat in case window update is ignored.
        // recordFramePainted will later refine this with a more accurate delay if a paint occurs.
        const auto delayMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_minFrameTime)
                                 .count();
        m_heartbeatTimer.start(static_cast<int>(delayMs));
    }
}

std::chrono::nanoseconds FrameManager::getJitterTolerance() const
{
    // Use 25% of the frame time as jitter tolerance, but cap it at 8ms.
    // This ensures we are "ready early" for VSync at 60Hz (4ms) while avoiding
    // over-rendering at very high frame rates.
    const auto tolerance = m_minFrameTime / 4;
    return std::min(tolerance, std::chrono::nanoseconds(std::chrono::milliseconds(8)));
}

std::chrono::nanoseconds FrameManager::getTimeUntilNextFrame() const
{
    if (m_lastUpdateTime.time_since_epoch().count() == 0) {
        return std::chrono::nanoseconds::zero();
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = now - m_lastUpdateTime;
    const auto tolerance = getJitterTolerance();
    if (elapsed + tolerance >= m_minFrameTime) {
        return std::chrono::nanoseconds::zero();
    }
    return m_minFrameTime - (elapsed + tolerance);
}

void FrameManager::recordFramePainted()
{
    // The previous frame just finished painting. Schedule the next one.
    if (needsHeartbeat()) {
        requestFrame();
    }
}

void FrameManager::requestFrame()
{
    const auto delay = getTimeUntilNextFrame();
    if (delay == std::chrono::nanoseconds::zero()) {
        // We are ready to paint now. Stop any pending timer and request an update.
        if (m_heartbeatTimer.isActive()) {
            m_heartbeatTimer.stop();
        }
        m_window.update();
        // Start a fallback timer in case window update is ignored.
        if (needsHeartbeat()) {
            const auto delayMs
                = std::chrono::duration_cast<std::chrono::milliseconds>(m_minFrameTime).count();
            m_heartbeatTimer.start(static_cast<int>(delayMs));
        }
    } else {
        // Start or restart the timer with the accurate delay.
        const int finalDelay = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(delay).count());

        // Only restart the timer if it's not active or if the new delay is significantly different.
        // This avoids hammering the timer during high-frequency input like mouse moves.
        if (!m_heartbeatTimer.isActive()
            || std::abs(m_heartbeatTimer.remainingTime() - finalDelay) > 1) {
            m_heartbeatTimer.start(std::max(1, finalDelay));
        }
    }
}

void FrameManager::updateMinFrameTime()
{
    const float targetFps = getConfig().canvas.advanced.maximumFps.getFloat();
    m_minFrameTime = std::chrono::nanoseconds(
        static_cast<long long>(1000000000.0 / static_cast<double>(std::max(1.0f, targetFps))));
}
