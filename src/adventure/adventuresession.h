#pragma once

#include <chrono>
#include <QString>

class AdventureSession
{
    template<typename T>
    struct Counter
    {
        virtual ~Counter() = default;
        virtual bool uninitialized() = 0;
        void update(T val)
        {
            if (uninitialized()) {
                initial = val;
                checkpoint = val;
            }
            current = val;
        }
        T checkpointGained()
        {
            T gained = checkpoint - current;
            checkpoint = current;
            return gained;
        }

        T initial, current;

    private:
        T checkpoint;
    };

    struct PointCounter : Counter<double>
    {
        virtual ~PointCounter() override = default;
        bool uninitialized() override;
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
    PointCounter tp() const;
    PointCounter xp() const;
    double calculateHourlyRateTP() const;
    double calculateHourlyRateXP() const;

private:
    double elapsedSeconds() const;

    QString m_charName;
    std::chrono::steady_clock::time_point m_startTimePoint, m_endTimePoint;
    bool m_isEnded;
    PointCounter m_xp, m_tp;
};
