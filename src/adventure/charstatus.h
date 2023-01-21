#pragma once

#include <QString>

class CharStatus
{
public:
    CharStatus(QString charName);

    QString name() const;
    double xpInitial() const;
    double xpCurrent() const;

    void updateXP(double xpCurrent);
    double checkpointXP();

private:
    QString m_CharName;

    double m_xpInitial, m_xpCheckpoint, m_xpCurrent;
};
