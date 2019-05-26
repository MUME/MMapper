/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "mumeclockwidget.h"

#include <cassert>
#include <QLabel>
#include <QString>

#include "../configuration/configuration.h"
#include "mumeclock.h"
#include "mumemoment.h"

MumeClockWidget::MumeClockWidget(MumeClock *clock, QWidget *parent)
    : QWidget(parent)
    , m_clock(clock)
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    assert(testAttribute(Qt::WA_DeleteOnClose));

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MumeClockWidget::updateLabel);
    m_timer->start(1000);

    updateLabel();
}

MumeClockWidget::~MumeClockWidget()
{
    delete m_timer;
}

void MumeClockWidget::updateLabel()
{
    // Ensure we have updated the epoch
    setConfig().mumeClock.startEpoch = m_clock->getMumeStartEpoch();

    // Hide or show the widget if necessary
    if (!getConfig().mumeClock.display) {
        hide();
        // Slow down the interval to a reasonable number
        m_timer->setInterval(60 * 1000);
        return;
    }
    if (m_timer->interval() != 1000) {
        show();
        // Speed up the interval again if the display needs to be shown
        m_timer->setInterval(1000);
    }

    MumeMoment moment = m_clock->getMumeMoment();
    MumeClockPrecision precision = m_clock->getPrecision();

    MumeMoonVisibility moonVisibility = moment.toMoonVisibility();
    QString moonStyleSheet = (moonVisibility == MumeMoonVisibility::MOON_INVISIBLE)
                                 ? "color:black;background:grey"
                                 : "color:black;background:white";
    moonPhaseLabel->setStyleSheet(moonStyleSheet);

    MumeMoonPhase phase = moment.toMoonPhase();
    switch (phase) {
    case MumeMoonPhase::PHASE_WAXING_CRESCENT:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x92"));
        break;
    case MumeMoonPhase::PHASE_FIRST_QUARTER:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x93"));
        break;
    case MumeMoonPhase::PHASE_WAXING_GIBBOUS:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x94"));
        break;
    case MumeMoonPhase::PHASE_FULL_MOON:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x95"));
        break;
    case MumeMoonPhase::PHASE_WANING_GIBBOUS:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x96"));
        break;
    case MumeMoonPhase::PHASE_THIRD_QUARTER:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x97"));
        break;
    case MumeMoonPhase::PHASE_WANING_CRESCENT:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x98"));
        break;
    case MumeMoonPhase::PHASE_NEW_MOON:
        moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x91"));
        break;
    case MumeMoonPhase::PHASE_UNKNOWN:
        moonPhaseLabel->setText("");
        break;
    }
    moonPhaseLabel->setStatusTip(moment.toMumeMoonTime());

    seasonLabel->setStatusTip(m_clock->toMumeTime(moment));
    MumeSeason season = moment.toSeason();
    if (season != m_lastSeason) {
        m_lastSeason = season;
        QString styleSheet = "color:black";
        QString text = "Unknown";
        switch (season) {
        case MumeSeason::SEASON_WINTER:
            styleSheet = "color:black;background:white";
            text = "Winter";
            break;
        case MumeSeason::SEASON_SPRING:
            styleSheet = "color:white;background:teal";
            text = "Spring";
            break;
        case MumeSeason::SEASON_SUMMER:
            styleSheet = "color:white;background:green";
            text = "Summer";
            break;
        case MumeSeason::SEASON_AUTUMN:
            styleSheet = "color:black;background:orange";
            text = "Autumn";
            break;
        case MumeSeason::SEASON_UNKNOWN:
        default:
            break;
        }
        seasonLabel->setStyleSheet(styleSheet);
        seasonLabel->setText(text);
    }
    MumeTime time = moment.toTimeOfDay();
    if (time != m_lastTime || precision != m_lastPrecision) {
        m_lastTime = time;
        m_lastPrecision = precision;
        // The current time is 12:15 am.
        QString styleSheet = "";
        QString statusTip = "";
        if (precision <= MumeClockPrecision::MUMECLOCK_DAY) {
            statusTip = "Please run \"time\" to sync the clock";
        } else if (time == MumeTime::TIME_DAWN) {
            styleSheet = "color:white;background:red";
            statusTip = "Ticks left until day";
        } else if (time >= MumeTime::TIME_DUSK) {
            styleSheet = "color:white;background:blue";
            statusTip = "Ticks left until day";
        } else {
            styleSheet = "color:black;background:yellow";
            statusTip = "Ticks left until night";
        }
        timeLabel->setStyleSheet(styleSheet);
        timeLabel->setStatusTip(statusTip);
    }
    timeLabel->setText(m_clock->toCountdown(moment));
}
