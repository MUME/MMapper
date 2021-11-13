// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeclockwidget.h"

#include <cassert>
#include <memory>
#include <QDateTime>
#include <QLabel>
#include <QMouseEvent>
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

    m_timer = std::make_unique<QTimer>(this);
    connect(m_timer.get(), &QTimer::timeout, this, &MumeClockWidget::slot_updateLabel);
    m_timer->start(1000);

    slot_updateLabel();
}

MumeClockWidget::~MumeClockWidget() = default;

void MumeClockWidget::mousePressEvent(QMouseEvent *event)
{
    // Force precision to minute and reset last sync to current timestamp
    m_clock->setPrecision(MumeClockPrecisionEnum::MINUTE);
    m_clock->setLastSyncEpoch(QDateTime::currentDateTimeUtc().toSecsSinceEpoch());

    slot_updateLabel();
}

void MumeClockWidget::slot_updateLabel()
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

    const MumeMoment moment = m_clock->getMumeMoment();
    const MumeClockPrecisionEnum precision = m_clock->getPrecision();

    bool updateMoonText = false;
    const MumeMoonPhaseEnum phase = moment.toMoonPhase();
    if (phase != m_lastPhase) {
        m_lastPhase = phase;
        switch (phase) {
        case MumeMoonPhaseEnum::WAXING_CRESCENT:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x92"));
            break;
        case MumeMoonPhaseEnum::FIRST_QUARTER:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x93"));
            break;
        case MumeMoonPhaseEnum::WAXING_GIBBOUS:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x94"));
            break;
        case MumeMoonPhaseEnum::FULL_MOON:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x95"));
            break;
        case MumeMoonPhaseEnum::WANING_GIBBOUS:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x96"));
            break;
        case MumeMoonPhaseEnum::THIRD_QUARTER:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x97"));
            break;
        case MumeMoonPhaseEnum::WANING_CRESCENT:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x98"));
            break;
        case MumeMoonPhaseEnum::NEW_MOON:
            moonPhaseLabel->setText(QString::fromUtf8("\xf0\x9f\x8c\x91"));
            break;
        case MumeMoonPhaseEnum::UNKNOWN:
            moonPhaseLabel->setText("");
            break;
        }
        updateMoonText = true;
    }

    seasonLabel->setStatusTip(m_clock->toMumeTime(moment));
    const MumeSeasonEnum season = moment.toSeason();
    if (season != m_lastSeason) {
        m_lastSeason = season;
        QString styleSheet = "color:black";
        QString text = "Unknown";
        switch (season) {
        case MumeSeasonEnum::WINTER:
            styleSheet = "color:black;background:white";
            text = "Winter";
            break;
        case MumeSeasonEnum::SPRING:
            styleSheet = "color:white;background:teal";
            text = "Spring";
            break;
        case MumeSeasonEnum::SUMMER:
            styleSheet = "color:white;background:green";
            text = "Summer";
            break;
        case MumeSeasonEnum::AUTUMN:
            styleSheet = "color:black;background:orange";
            text = "Autumn";
            break;
        case MumeSeasonEnum::UNKNOWN:
        default:
            break;
        }
        seasonLabel->setStyleSheet(styleSheet);
        seasonLabel->setText(text);
    }

    bool updateMoonStyleSheet = false;
    const MumeTimeEnum time = moment.toTimeOfDay();
    if (time != m_lastTime || precision != m_lastPrecision) {
        m_lastTime = time;
        m_lastPrecision = precision;
        // The current time is 12:15 am.
        QString styleSheet = "";
        QString statusTip = "";
        if (precision <= MumeClockPrecisionEnum::UNSET) {
            styleSheet = "color:white;background:grey";
            statusTip = "The clock has not synced with MUME! Click to override at your own risk.";
        } else if (time == MumeTimeEnum::DAWN) {
            styleSheet = "color:white;background:red";
            statusTip = "Ticks left until day";
        } else if (time >= MumeTimeEnum::DUSK) {
            styleSheet = "color:white;background:blue";
            statusTip = "Ticks left until day";
        } else {
            styleSheet = "color:black;background:yellow";
            statusTip = "Ticks left until night";
        }
        timeLabel->setStyleSheet(styleSheet);
        timeLabel->setStatusTip(statusTip);
        updateMoonStyleSheet = true;
    }
    if (precision <= MumeClockPrecisionEnum::DAY) {
        // Prepend warning emoji to countdown
        timeLabel->setText(QString::fromUtf8("\xe2\x9a\xa0").append(m_clock->toCountdown(moment)));
    } else
        timeLabel->setText(m_clock->toCountdown(moment));

    const MumeMoonVisibilityEnum moonVisibility = moment.toMoonVisibility();
    if (moonVisibility != m_lastVisibility || updateMoonStyleSheet) {
        m_lastVisibility = moonVisibility;
        const QString moonStyleSheet = (moonVisibility == MumeMoonVisibilityEnum::HIDDEN)
                                           ? "color:black;background:grey"
                                           : ((moment.isMoonBright() && time >= MumeTimeEnum::DUSK)
                                                  ? "color:black;background:yellow"
                                                  : "color:black;background:white");
        moonPhaseLabel->setStyleSheet(moonStyleSheet);
        updateMoonText = true;
    }

    if (updateMoonText)
        moonPhaseLabel->setStatusTip(moment.toMumeMoonTime());
}
