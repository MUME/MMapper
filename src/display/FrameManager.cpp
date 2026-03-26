// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "FrameManager.h"

#include "../configuration/configuration.h"

#include <algorithm>

#include <QOpenGLWindow>

FrameManager::FrameManager(QOpenGLWindow &window, Legacy::UboManager &uboManager, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_uboManager(uboManager)
{
    m_uboManager.registerRebuildFunction(Legacy::SharedVboEnum::TimeBlock,
                                         [this](Legacy::Functions &gl) {
                                             m_uboManager.sync<Legacy::SharedVboEnum::TimeBlock>(gl);
                                         });

    updateMinFrameTime();
    setConfig().canvas.advanced.maximumFps.registerChangeCallback(m_configLifetime, [this]() {
        updateMinFrameTime();
    });

    m_heartbeatTimer.setSingleShot(true);
    m_heartbeatTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &FrameManager::onHeartbeat);

    requestFrame();
}

FrameManager::~FrameManager()
{
    m_uboManager.unregisterRebuildFunction(Legacy::SharedVboEnum::TimeBlock);
}

void FrameManager::registerCallback(const Signal2Lifetime &lifetime, AnimationCallback callback)
{
    m_callbacks.push_back({lifetime.getObj(), std::move(callback)});
}

bool FrameManager::needsHeartbeat()
{
    // NOTE:
    // needsHeartbeat() is called from multiple code paths (e.g. beginFrame(),
    // onHeartbeat(), and the recordFramePainted()/requestFrame() paths), and each
    // call walks and may invoke registered animation callbacks.
    //
    // As a result, callbacks can be evaluated multiple times within a single
    // frame/heartbeat decision. Callbacks used with FrameManager *must* therefore:
    //   - be cheap to execute,
    //   - be idempotent, and
    //   - avoid externally visible side effects beyond returning AnimationStatusEnum.
    //
    // Their sole responsibility here is to report whether they wish the heartbeat
    // to continue or can safely stop.

    if (m_dirty) {
        return true;
    }

    bool anyActive = false;
    bool needsCleanup = false;

    for (const auto &entry : m_callbacks) {
        if (auto shared = entry.lifetime.lock()) {
            if (entry.callback() == AnimationStatusEnum::Continue) {
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

void FrameManager::cleanupExpiredCallbacks()
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
        if (elapsed < m_minFrameTime) {
            return std::nullopt;
        }
    }

    // Deduplication: skip frame if nothing has changed and we aren't animating.
    if (!m_dirty && !needsHeartbeat()) {
        return std::nullopt;
    }
    m_dirty = false;

    // Calculate delta time
    float deltaTime = 0.0f;
    if (hasLastUpdate) {
        const auto elapsed = now - m_lastUpdateTime;
        deltaTime = std::chrono::duration<float>(elapsed).count();
    }
    m_lastUpdateTime = now;

    // Cap deltaTime for simulation to match map movement during dragging and avoid quantization jitter.
    // Cap at 1.0s to avoid huge jumps after window focus loss or lag, while supporting low FPS.
    auto lastFrameDeltaTime = std::min(deltaTime, 1.0f);

    // Refresh internal struct for UBO
    m_elapsedTime += lastFrameDeltaTime;
    auto &timeBlock = m_uboManager.get<Legacy::SharedVboEnum::TimeBlock>();
    timeBlock.time = glm::vec4(m_elapsedTime, lastFrameDeltaTime, 0.0f, 0.0f);

    if (lastFrameDeltaTime > 0.0f) {
        m_uboManager.invalidate(Legacy::SharedVboEnum::TimeBlock);
    }

    return Frame(*this);
}

float FrameManager::getElapsedTime() const
{
    return m_elapsedTime;
}

void FrameManager::onHeartbeat()
{
    if (!needsHeartbeat()) {
        m_heartbeatTimer.stop();
        return;
    }

    m_window.update();

    if (needsHeartbeat()) {
        requestFrame();
    }
}

std::chrono::nanoseconds FrameManager::getTimeUntilNextFrame() const
{
    if (m_lastUpdateTime.time_since_epoch().count() == 0) {
        return std::chrono::nanoseconds::zero();
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = now - m_lastUpdateTime;
    if (elapsed >= m_minFrameTime) {
        return std::chrono::nanoseconds::zero();
    }
    return m_minFrameTime - elapsed;
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
            const int delayMs = static_cast<int>(
                std::chrono::ceil<std::chrono::milliseconds>(m_minFrameTime).count());
            m_heartbeatTimer.start(delayMs);
        }
    } else {
        // Start or restart the timer with the accurate delay.
        const int finalDelay = static_cast<int>(
            std::chrono::ceil<std::chrono::milliseconds>(delay).count());

        // Only restart the timer if it's not active or if the new delay is significantly different.
        // This avoids hammering the timer during high-frequency input like mouse moves.
        if (!m_heartbeatTimer.isActive()
            || std::abs(m_heartbeatTimer.remainingTime() - finalDelay) > 1) {
            m_heartbeatTimer.start(finalDelay);
        }
    }
}

void FrameManager::updateMinFrameTime()
{
    const float targetFps = getConfig().canvas.advanced.maximumFps.getFloat();
    m_minFrameTime = std::chrono::nanoseconds(
        static_cast<long long>(1000000000.0 / static_cast<double>(std::max(1.0f, targetFps))));
}
