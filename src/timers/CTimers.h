#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "../global/macros.h"

#include <cstdint>
#include <list>
#include <string>

#include <QMutex>
#include <QObject>
#include <QTimer>

class NODISCARD TTimer final
{
private:
    std::string m_name;
    std::string m_desc;
    int64_t m_start = 0;
    int64_t m_duration = 0;

public:
    explicit TTimer(const std::string &name, const std::string &desc, int64_t durationMs);
    explicit TTimer(const std::string &name, const std::string &desc);

public:
    NODISCARD const std::string &getName() const { return m_name; }
    NODISCARD const std::string &getDescription() const { return m_desc; }

public:
    NODISCARD int64_t durationMs() const { return m_duration; }
    NODISCARD int64_t elapsedMs() const;
};

class CTimers final : public QObject
{
    Q_OBJECT

private:
    QMutex m_lock;
    std::list<TTimer> m_timers;
    std::list<TTimer> m_countdowns;
    QTimer m_timer;

public:
    NODISCARD std::string getTimers();
    NODISCARD std::string getCountdowns();

signals:
    void sig_sendTimersUpdateToUser(const std::string &str);

public:
    explicit CTimers(QObject *parent);
    ~CTimers() final = default;

    void addTimer(const std::string &name, const std::string &desc);
    void addCountdown(const std::string &name, const std::string &desc, int64_t timeMs);

    bool removeTimer(const std::string &name);
    bool removeCountdown(const std::string &name);

    // for stat command representation
    NODISCARD std::string getStatCommandEntry();

    void clear();

public slots:
    void slot_finishCountdownTimer();
};
