#pragma once

#include <QString>

class AdventureProgress
{
public:
    AdventureProgress(QString charName);

    QString name() const;
    double xpInitial() const;
    double xpCurrent() const;

    double checkpointXPGained();
    double peekXPGained();
    void updateXP(double xp);

private:
    QString m_CharName;

    double m_xpInitial, m_xpCheckpoint, m_xpCurrent;
};
