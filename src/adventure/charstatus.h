#pragma once

#include <QString>

class AdventureProgress
{
public:
    AdventureProgress(QString charName);

    QString name() const;
    double xpInitial() const;
    double xpCurrent() const;

    double checkpointXP();
    void updateXP(double xpCurrent);

private:
    QString m_CharName;

    double m_xpInitial, m_xpCheckpoint, m_xpCurrent;
};
