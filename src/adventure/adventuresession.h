#pragma once

#include <QString>

class AdventureSession
{
public:
    AdventureSession(QString charName);

    AdventureSession(const AdventureSession &src);
    AdventureSession &operator=(const AdventureSession &src);

    double checkpointXPGained();
    void updateXP(double xp);
    void endSession();

    QString name() const;
    std::chrono::steady_clock::time_point startTime() const;
    std::chrono::steady_clock::time_point endTime() const;
    bool isEnded() const;
    double xpInitial() const;
    double xpCurrent() const;
    double calculateHourlyRateXP() const;

private:
    QString m_charName;
    std::chrono::steady_clock::time_point m_startTimePoint, m_endTimePoint;
    bool m_isEnded;
    double m_xpInitial, m_xpCheckpoint, m_xpCurrent;
};
