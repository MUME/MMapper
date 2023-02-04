#pragma once

#include <chrono>
#include <QDebug>
#include <QString>

class AdventureSession
{
    template<typename T>
    struct Counter
    {
        T start, current;

        void update(T val)
        {
            if (!initialized) {
                start = val;
                lastCheckpoint = val;
                initialized = true;
            }
            current = val;
        }
        T checkpoint()
        {
            T gained = current - lastCheckpoint;
            lastCheckpoint = current;
            return gained;
        }
        T gainedSession() const { return current - start; }

    private:
        bool initialized = false;
        T lastCheckpoint;
    };

public:
    AdventureSession(QString charName);

    AdventureSession(const AdventureSession &src);
    AdventureSession &operator=(const AdventureSession &src);

    double checkpointTPGained();
    double checkpointXPGained();
    void updateTP(double tp);
    void updateXP(double xp);
    void endSession();

    QString name() const;
    std::chrono::steady_clock::time_point startTime() const;
    std::chrono::steady_clock::time_point endTime() const;
    bool isEnded() const;
    Counter<double> tp() const;
    Counter<double> xp() const;
    double calculateHourlyRateTP() const;
    double calculateHourlyRateXP() const;

private:
    double elapsedSeconds() const;

    QString m_charName;
    std::chrono::steady_clock::time_point m_startTimePoint, m_endTimePoint;
    bool m_isEnded;
    Counter<double> m_tp, m_xp;
};
