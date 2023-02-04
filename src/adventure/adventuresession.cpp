#include "adventuresession.h"
#include "QtCore/qdebug.h"
#include "adventuretracker.h"
#include "adventurewidget.h"
#include "xpstatuswidget.h"

bool AdventureSession::PointCounter::uninitialized()
{
    // A class with virtuals needs at least one non-inline (i.e. source file defined) method
    // to avoid clang -Wweak-vtables, which seems to be a controversial warning in general.
    // https://stackoverflow.com/q/28786473/614880
    return initial == 0.0;
}

AdventureSession::AdventureSession(QString charName)
    : m_charName{charName}
    , m_startTimePoint{std::chrono::steady_clock::now()}
    , m_endTimePoint{}
    , m_isEnded{false}
    , m_xp{}
{}

AdventureSession::AdventureSession(const AdventureSession &src)
    : m_charName{src.m_charName}
    , m_startTimePoint{src.m_startTimePoint}
    , m_endTimePoint{src.m_endTimePoint}
    , m_isEnded{src.m_isEnded}
    , m_xp{src.m_xp}
{}

AdventureSession &AdventureSession::operator=(const AdventureSession &src)
{
    m_charName = src.m_charName;
    m_startTimePoint = src.m_startTimePoint;
    m_endTimePoint = src.m_endTimePoint;
    m_isEnded = src.m_isEnded;
    m_xp = src.m_xp;

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

AdventureSession::PointCounter AdventureSession::tp() const
{
    return m_tp;
}

AdventureSession::PointCounter AdventureSession::xp() const
{
    return m_xp;
}

double AdventureSession::checkpointTPGained()
{
    return m_tp.checkpointGained();
}

double AdventureSession::checkpointXPGained()
{
    return m_xp.checkpointGained();
}

void AdventureSession::updateTP(double tp)
{
    m_tp.update(tp);
}

void AdventureSession::updateXP(double xp)
{
    m_xp.update(xp);
}

double AdventureSession::calculateHourlyRateTP() const
{
    auto tpSessionPerSecond = (m_tp.current - m_tp.initial) / elapsedSeconds();
    return tpSessionPerSecond * 3600;
}

double AdventureSession::calculateHourlyRateXP() const
{
    auto xpSessionPerSecond = (m_xp.current - m_xp.initial) / elapsedSeconds();
    return xpSessionPerSecond * 3600;
}

double AdventureSession::elapsedSeconds() const
{
    auto start = m_startTimePoint;
    auto end = std::chrono::steady_clock::now();
    if (m_isEnded)
        end = m_endTimePoint;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    return static_cast<double>(elapsed.count());
}
