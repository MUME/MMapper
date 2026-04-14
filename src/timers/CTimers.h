#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <cstdint>
#include <list>
#include <string>

#include <QObject>
#include <QTimer>

class NODISCARD TTimer final
{
private:
    std::string m_name;
    std::string m_desc;
    int64_t m_start = 0;
    int64_t m_duration = 0;
    bool m_expired = false;

public:
    explicit TTimer(std::string name, std::string desc, int64_t durationMs);
    explicit TTimer(std::string name, std::string desc);
    TTimer() = delete;
    ~TTimer() = default;
    DEFAULT_MOVE_CTOR(TTimer);
    DEFAULT_COPY_CTOR(TTimer);
    DEFAULT_MOVE_ASSIGN_OP(TTimer);
    DEFAULT_COPY_ASSIGN_OP(TTimer);

public:
    NODISCARD const std::string &getName() const { return m_name; }
    NODISCARD const std::string &getDescription() const { return m_desc; }

public:
    NODISCARD int64_t durationMs() const { return m_duration; }
    NODISCARD int64_t elapsedMs() const;
    NODISCARD int64_t remainingMs() const;
    NODISCARD int64_t startTimeMs() const { return m_start; }

    NODISCARD bool isCountdown() const { return m_duration > 0; }
    NODISCARD bool isExpired() const { return m_expired; }
    void setExpired(bool expired) { m_expired = expired; }
};

class NODISCARD_QOBJECT CTimers final : public QObject
{
    Q_OBJECT

private:
    std::list<TTimer> m_allTimers;
    QTimer m_timer;

public:
    NODISCARD std::string getTimers() const;
    NODISCARD std::string getCountdowns() const;

public:
    explicit CTimers(QObject *parent);
    ~CTimers() final = default;
    DELETE_CTORS_AND_ASSIGN_OPS(CTimers);

    void addTimer(std::string name, std::string desc);
    void addCountdown(std::string name, std::string desc, int64_t timeMs);

    NODISCARD bool removeTimer(const std::string &name);
    NODISCARD bool removeCountdown(const std::string &name);

    void stopTimer(const std::string &name);
    void stopCountdown(const std::string &name);
    void resetTimer(const std::string &name);
    void resetCountdown(const std::string &name);

    void clearExpired();

    // for stat command representation
    NODISCARD std::string getStatCommandEntry() const;

    void clear();

    void moveTimer(int from, int to);

    NODISCARD const std::list<TTimer> &allTimers() const { return m_allTimers; }

signals:
    void sig_sendTimersUpdateToUser(const std::string &str);
    void sig_timerAdded();
    void sig_timerRemoved();
    void sig_timersUpdated();

public slots:
    void slot_finishCountdownTimer();

private:
    void restartCountdownTimer();
};
