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

#include <QSettings>
#include <QTimer>

#include "mumeclockwidget.h"
#include "configuration.h"

MumeClockWidget::MumeClockWidget(MumeClock *clock, QWidget *parent)
    : QWidget(parent),
      m_clock(clock)
{
    setupUi(this);

    m_lastSeason = SEASON_UNKNOWN;
    m_lastTime = TIME_UNKNOWN;
    m_lastPrecision = MUMECLOCK_UNSET;

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateLabel()));
    m_timer->start(1000);

    updateLabel();
}

MumeClockWidget::~MumeClockWidget()
{
    if (m_timer) delete m_timer;
}

void MumeClockWidget::updateLabel()
{
    // Ensure we have updated the epoch
    Config().m_mumeStartEpoch = m_clock->getMumeStartEpoch();

    // Hide or show the widget if necessary
    if (!Config().m_displayMumeClock) {
        hide();
        // Slow down the interval to a reasonable number
        m_timer->setInterval(60 * 1000);
        return ;
    } else if (m_timer->interval() != 1000) {
        show();
        // Speed up the interval again if the display needs to be shown
        m_timer->setInterval(1000);
    }

    MumeMoment moment = m_clock->getMumeMoment();
    MumeClockPrecision precision = m_clock->getPrecision();

    seasonLabel->setStatusTip(m_clock->toMumeTime(moment));

    MumeSeason season = moment.toSeason();
    if (season != m_lastSeason) {
        m_lastSeason = season;
        QString styleSheet = "color:black";
        QString text = "Unknown";
        switch (season) {
        case SEASON_WINTER:
            styleSheet = "color:black;background:white";
            text = "Winter";
            break;
        case SEASON_SPRING:
            styleSheet = "color:white;background:teal";
            text = "Spring";
            break;
        case SEASON_SUMMER:
            styleSheet = "color:white;background:green";
            text = "Summer";
            break;
        case SEASON_AUTUMN:
            styleSheet = "color:black;background:orange";
            text = "Autumn";
            break;
        case SEASON_UNKNOWN:
        default:
            break;
        }
        seasonLabel->setStyleSheet(styleSheet);
        seasonLabel->setText(text);
    }

    MumeTime time = moment.toTimeOfDay();
    if (time != m_lastTime || precision != m_lastPrecision) {
        m_lastTime = time;
        // The current time is 12:15 am.
        QString styleSheet = "color:black";
        QString statusTip = "";
        if (precision <= MUMECLOCK_DAY) {
            styleSheet = "color:black";
            statusTip = "Please run \"time\" to sync the clock";

        } else if (time == TIME_DAWN) {
            styleSheet = "color:white;background:red";
            statusTip = "Ticks left until day";
        } else if (time >= TIME_DUSK) {
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
