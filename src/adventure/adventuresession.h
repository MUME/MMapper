#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/macros.h"

#include <chrono>

#include <QString>

class TestAdventure;

class NODISCARD AdventureSession final
{
    friend TestAdventure;

public:
    using Clock = std::chrono::steady_clock;

private:
    template<typename T>
    struct NODISCARD Counter final
    {
    public:
        T start{}, current{};

    private:
        bool initialized = false;
        T lastCheckpoint{};

    public:
        NODISCARD
        T checkpoint()
        {
            T gained = current - lastCheckpoint;
            lastCheckpoint = current;
            return gained;
        }
        NODISCARD
        T gainedSession() const { return current - start; }
        void update(T val)
        {
            if (!initialized) {
                start = val;
                lastCheckpoint = val;
                initialized = true;
            }
            current = val;
        }
    };

private:
    QString m_charName;
    Clock::time_point m_startTimePoint{}, m_endTimePoint{};
    bool m_isEnded = false;
    Counter<double> m_tp, m_xp;

public:
    explicit AdventureSession(QString charName);
    ~AdventureSession() = default;
    AdventureSession(const AdventureSession &src) = delete;
    AdventureSession &operator=(const AdventureSession &src) = delete;

    NODISCARD double checkpointXPGained();
    void updateTP(double tp);
    void updateXP(double xp);
    void endSession();

    NODISCARD const QString &name() const { return m_charName; }
    NODISCARD Clock::time_point startTime() const { return m_startTimePoint; }
    NODISCARD Clock::time_point endTime() const { return m_endTimePoint; }
    NODISCARD bool isEnded() const { return m_isEnded; }
    NODISCARD Counter<double> tp() const { return m_tp; }
    NODISCARD Counter<double> xp() const { return m_xp; }
    NODISCARD double calculateHourlyRateTP() const;
    NODISCARD double calculateHourlyRateXP() const;
    NODISCARD static QString formatPoints(double points);

private:
    NODISCARD double calculateHourlyRate(double points) const;
    NODISCARD std::chrono::seconds elapsed() const;
};
