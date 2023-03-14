#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include <chrono>
#include <QString>
#include <QtCore>

class AdventureSession
{
    template<typename T>
    struct Counter
    {
        T start, current;

        T checkpoint()
        {
            T gained = current - lastCheckpoint;
            lastCheckpoint = current;
            return gained;
        }
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

    private:
        bool initialized = false;
        T lastCheckpoint;
    };

public:
    explicit AdventureSession(QString charName);

    AdventureSession(const AdventureSession &src);
    AdventureSession &operator=(const AdventureSession &src);

    double checkpointXPGained();
    void updateTP(double tp);
    void updateXP(double xp);
    void endSession();

    const QString &name() const;
    std::chrono::steady_clock::time_point startTime() const;
    std::chrono::steady_clock::time_point endTime() const;
    bool isEnded() const;
    Counter<double> tp() const;
    Counter<double> xp() const;
    double calculateHourlyRateTP() const;
    double calculateHourlyRateXP() const;
    static const QString formatPoints(double points);

private:
    double calculateHourlyRate(double points) const;
    std::chrono::seconds elapsed() const;

    QString m_charName;
    std::chrono::steady_clock::time_point m_startTimePoint, m_endTimePoint;
    bool m_isEnded;
    Counter<double> m_tp, m_xp;
};
