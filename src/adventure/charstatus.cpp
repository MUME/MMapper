#include "charstatus.h"
#include "QtCore/qdebug.h"

CharStatus::CharStatus(QString charName)
    : m_CharName{charName}
    , m_xpInitial{0.0}
    , m_xpCheckpoint{0.0}
    , m_xpCurrent{0.0}
{}

QString CharStatus::name() const
{
    return m_CharName;
}

double CharStatus::xpInitial() const
{
    return m_xpInitial;
}

double CharStatus::xpCurrent() const
{
    return m_xpCurrent;
}

void CharStatus::updateXP(double xpCurrent)
{
    if (m_xpInitial == 0.0) {
        qDebug().noquote() << QString("Adventure: initial XP: %1")
                                  .arg(QString::number(xpCurrent, 'f', 0));

        m_xpInitial = xpCurrent;
        m_xpCheckpoint = xpCurrent;
    }

    m_xpCurrent = xpCurrent;
}

double CharStatus::checkpointXP()
{
    double xpGained = m_xpCurrent - m_xpCheckpoint;
    m_xpCheckpoint = m_xpCurrent;

    return xpGained;
}
