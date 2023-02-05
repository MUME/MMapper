#include "adventuresession.h"
#include "QtCore/qdebug.h"
#include "xpstatuswidget.h"
#include <QtWidgets>

AdventureSession::AdventureSession(QString charName)
    : m_charName{charName}
    , m_startTimePoint{std::chrono::steady_clock::now()}
    , m_endTimePoint{}
    , m_isEnded{false}
    , m_tp{}
    , m_xp{}
{}

AdventureSession::AdventureSession(const AdventureSession &src)
    : m_charName{src.m_charName}
    , m_startTimePoint{src.m_startTimePoint}
    , m_endTimePoint{src.m_endTimePoint}
    , m_isEnded{src.m_isEnded}
    , m_tp{src.m_tp}
    , m_xp{src.m_xp}
{}

AdventureSession &AdventureSession::operator=(const AdventureSession &src)
{
    m_charName = src.m_charName;
    m_startTimePoint = src.m_startTimePoint;
    m_endTimePoint = src.m_endTimePoint;
    m_isEnded = src.m_isEnded;
    m_tp = src.m_tp;
    m_xp = src.m_xp;

    return *this;
}

void AdventureSession::endSession()
{
    if (!m_isEnded) {
        m_endTimePoint = std::chrono::steady_clock::now();
        m_isEnded = true;
    }
}

const QString &AdventureSession::name() const
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

AdventureSession::Counter<double> AdventureSession::tp() const
{
    return m_tp;
}

AdventureSession::Counter<double> AdventureSession::xp() const
{
    return m_xp;
}

double AdventureSession::checkpointTPGained()
{
    return m_tp.checkpoint();
}

double AdventureSession::checkpointXPGained()
{
    return m_xp.checkpoint();
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
    return calculateHourlyRate(m_tp.gainedSession());
}

double AdventureSession::calculateHourlyRateXP() const
{
    return calculateHourlyRate(m_xp.gainedSession());
}

double AdventureSession::calculateHourlyRate(double points) const
{
    return points / static_cast<double>(elapsed().count())
           * std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(1)).count();
}

std::chrono::seconds AdventureSession::elapsed() const
{
    auto end = m_isEnded ? m_endTimePoint : std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(end - m_startTimePoint);
}

const QString AdventureSession::formatPoints(double points)
{
    if (abs(points) < 1000) {
        return QString::number(points, 'f', 0);
    }

    if (abs(points) < (20 * 1000)) {
        return QString::number(points / 1000, 'f', 1) + "k";
    }

    return QString::number(points / 1000, 'f', 0) + "k";
}