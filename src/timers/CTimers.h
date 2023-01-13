#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include <cstdint>
#include <list>
#include <string>
#include <QMutex>
#include <QObject>
#include <QTimer>

class TTimer final
{
public:
    explicit TTimer(const std::string &name, const std::string &desc, int64_t durationMs);
    explicit TTimer(const std::string &name, const std::string &desc);

public:
    const std::string &getName() const { return name; }
    const std::string &getDescription() const { return desc; }

public:
    int64_t durationMs() const { return duration; }
    int64_t elapsedMs() const;

private:
    std::string name;
    std::string desc;
    int64_t start;
    int64_t duration;
};

class CTimers final : public QObject
{
    Q_OBJECT

private:
    QMutex m_lock;
    std::list<TTimer> m_timers;
    std::list<TTimer> m_countdowns;
    QTimer m_timer;

private:
    std::string getTimers();
    std::string getCountdowns();

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
    std::string getStatCommandEntry();

    void clear();

public slots:
    void slot_finishCountdownTimer();
};
