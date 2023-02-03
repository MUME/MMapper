#include "adventuresession.h"
#include "QtCore/qdebug.h"
#include "adventuretracker.h"
#include "adventurewidget.h"
#include "xpstatuswidget.h"

AdventureSession::AdventureSession(QString charName)
    : m_charName{charName}
    , m_startTimePoint{std::chrono::steady_clock::now()}
    , m_endTimePoint{}
    , m_isEnded{false}
    , m_xpInitial{0.0}
    , m_xpCheckpoint{0.0}
    , m_xpCurrent{0.0}
{}

AdventureSession::AdventureSession(const AdventureSession &src)
    : m_charName{src.m_charName}
    , m_startTimePoint{src.m_startTimePoint}
    , m_endTimePoint{src.m_endTimePoint}
    , m_isEnded{src.m_isEnded}
    , m_xpInitial{src.m_xpInitial}
    , m_xpCheckpoint{src.m_xpCheckpoint}
    , m_xpCurrent{src.m_xpCurrent}
{}

AdventureSession &AdventureSession::operator=(const AdventureSession &src)
{
    m_charName = src.m_charName;
    m_startTimePoint = src.m_startTimePoint;
    m_endTimePoint = src.m_endTimePoint;
    m_isEnded = src.m_isEnded;
    m_xpInitial = src.m_xpInitial;
    m_xpCheckpoint = src.m_xpCheckpoint;
    m_xpCurrent = src.m_xpCurrent;

    return *this;
}

void AdventureSession::endSession()
{
    m_isEnded = true;
    m_endTimePoint = std::chrono::steady_clock::now();
}

QString AdventureSession::name() const
{
    return m_charName;
}

std::chrono::steady_clock::time_point AdventureSession::startTime() const
{
    return m_startTimePoint;
}

std::chrono::steady_clock::time_point AdventureSession::endTime() const
{
    return m_endTimePoint;
}

bool AdventureSession::isEnded() const
{
    return m_isEnded;
}

double AdventureSession::xpInitial() const
{
    return m_xpInitial;
}

double AdventureSession::xpCurrent() const
{
    return m_xpCurrent;
}

double AdventureSession::checkpointXPGained()
{
    double xpGained = m_xpCurrent - m_xpCheckpoint;
    m_xpCheckpoint = m_xpCurrent;

    return xpGained;
}

void AdventureSession::updateXP(double xp)
{
    if (m_xpInitial == 0.0) {
        qDebug().noquote() << QString("Adventure: initial XP: %1").arg(QString::number(xp, 'f', 0));

        m_xpInitial = xp;
        m_xpCheckpoint = xp;
    }

    m_xpCurrent = xp;
}

double AdventureSession::calculateHourlyRateXP() const
{
    auto start = m_startTimePoint;
    auto end = std::chrono::steady_clock::now();
    if (m_isEnded)
        end = m_endTimePoint;
    auto elapsed = std::__1::chrono::duration_cast<std::chrono::seconds>(end - start);

    double xpAllSession = m_xpCurrent - m_xpInitial;
    double xpAllSessionPerSecond = xpAllSession / static_cast<double>(elapsed.count());

    qDebug().noquote() << QString("seconds %1 xp %2").arg(elapsed.count()).arg(xpAllSession);

    return xpAllSessionPerSecond * 3600;
}
