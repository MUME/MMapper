// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "adventuresession.h"

#include <QDebug>

AdventureSession::AdventureSession(QString charName)
    : m_charName{std::move(charName)}
    , m_startTimePoint{std::chrono::steady_clock::now()}
{}

void AdventureSession::endSession()
{
    if (!m_isEnded) {
        m_endTimePoint = Clock::now();
        m_isEnded = true;
    }
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

double AdventureSession::calculateHourlyRate(const double points) const
{
    const auto seconds = elapsed().count();
    return points / static_cast<double>(seconds) * 3600.0;
}

std::chrono::seconds AdventureSession::elapsed() const
{
    auto end = m_isEnded ? m_endTimePoint : Clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(end - m_startTimePoint);
}

QString AdventureSession::formatPoints(double points)
{
    if (abs(points) < 1000) {
        return QString::number(points, 'f', 0);
    }

    if (abs(points) < (20 * 1000)) {
        return QString::number(points / 1000, 'f', 1) + "k";
    }

    return QString::number(points / 1000, 'f', 0) + "k";
}
