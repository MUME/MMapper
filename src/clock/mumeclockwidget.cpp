// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeclockwidget.h"

#include "../configuration/configuration.h"
#include "ClockStrings.h"
#include "mumeclock.h"
#include "mumemoment.h"

#include <cassert>

#include <QDateTime>
#include <QLabel>
#include <QMouseEvent>
#include <QString>

MumeClockWidget::MumeClockWidget(GameObserver &observer, MumeClock &clock, QWidget *const parent)
    : QWidget(parent)
    , m_clock(clock)
{
    setupUi(this);
    moonPhaseLabel->setText("");
    seasonLabel->setText("");
    setAttribute(Qt::WA_DeleteOnClose);
    assert(testAttribute(Qt::WA_DeleteOnClose));
    setAttribute(Qt::WA_Hover, true);
    assert(testAttribute(Qt::WA_Hover));

    observer.sig2_timeOfDayChanged.connect(m_lifetime,
                                           [this](const MumeTimeEnum time) { updateTime(time); });
    observer.sig2_moonPhaseChanged.connect(m_lifetime, [this](MumeMoonPhaseEnum phase) {
        updateMoonPhase(phase);
    });
    observer.sig2_moonVisibilityChanged.connect(m_lifetime,
                                                [this](const MumeMoonVisibilityEnum visibility) {
                                                    updateMoonVisibility(visibility);
                                                });
    observer.sig2_seasonChanged.connect(m_lifetime, [this](const MumeSeasonEnum season) {
        updateSeason(season);
    });
    observer.sig2_weatherChanged.connect(m_lifetime, [this](const PromptWeatherEnum weather) {
        updateWeather(weather);
    });
    observer.sig2_fogChanged.connect(m_lifetime,
                                     [this](const PromptFogEnum fog) { updateFog(fog); });
    observer.sig2_tick.connect(m_lifetime,
                               [this](const MumeMoment &moment) { updateCountdown(moment); });

    auto moment = m_clock.getMumeMoment();
    updateTime(moment.toTimeOfDay());
    updateMoonPhase(moment.moonPhase());
    updateMoonVisibility(moment.moonVisibility());
    updateSeason(moment.toSeason());
    updateCountdown(moment);
    updateWeather(PromptWeatherEnum::NICE);
    updateFog(PromptFogEnum::NO_FOG);
}

MumeClockWidget::~MumeClockWidget() = default;

void MumeClockWidget::mousePressEvent(QMouseEvent * /*event*/)
{
    // Force precision to minute and reset last sync to current timestamp
    m_clock.setPrecision(MumeClockPrecisionEnum::MINUTE);
    m_clock.setLastSyncEpoch(QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
    auto moment = m_clock.getMumeMoment();
    updateTime(moment.toTimeOfDay());
    updateCountdown(moment);
    updateStatusTips(moment);
}

bool MumeClockWidget::event(QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        updateStatusTips(m_clock.getMumeMoment());
    }
    return QWidget::event(event);
}

void MumeClockWidget::updateTime(MumeTimeEnum time)
{
    // The current time is 12:15 am.
    QString styleSheet = "";
    if (m_clock.getPrecision() <= MumeClockPrecisionEnum::UNSET) {
        styleSheet = "padding-left:1px;padding-right:1px;color:white;background:grey";
    } else if (time == MumeTimeEnum::DAWN) {
        styleSheet = "padding-left:1px;padding-right:1px;color:white;background:red";
    } else if (time >= MumeTimeEnum::DUSK) {
        styleSheet = "padding-left:1px;padding-right:1px;color:white;background:blue";
    } else {
        styleSheet = "padding-left:1px;padding-right:1px;color:black;background:yellow";
    }

    timeLabel->setStyleSheet(styleSheet);
}

void MumeClockWidget::updateMoonPhase(MumeMoonPhaseEnum phase)
{
    moonPhaseLabel->setText(clockstrings::moonPhaseEmoji(phase));
}

void MumeClockWidget::updateMoonVisibility(MumeMoonVisibilityEnum visibility)
{
    const QString moonStyleSheet = (visibility == MumeMoonVisibilityEnum::INVISIBLE
                                    || visibility == MumeMoonVisibilityEnum::UNKNOWN)
                                       ? "color:black;background:grey"
                                   : (visibility == MumeMoonVisibilityEnum::BRIGHT)
                                       ? "color:black;background:yellow"
                                       : "color:black;background:white";
    moonPhaseLabel->setStyleSheet(moonStyleSheet);
}

void MumeClockWidget::updateSeason(MumeSeasonEnum season)
{
    QString styleSheet = "color:black";
    switch (season) {
    case MumeSeasonEnum::WINTER:
        styleSheet = "color:black;background:white";
        break;
    case MumeSeasonEnum::SPRING:
        styleSheet = "color:white;background:teal";
        break;
    case MumeSeasonEnum::SUMMER:
        styleSheet = "color:white;background:green";
        break;
    case MumeSeasonEnum::AUTUMN:
        styleSheet = "color:black;background:orange";
        break;
    case MumeSeasonEnum::UNKNOWN:
    default:
        break;
    }
    seasonLabel->setStyleSheet(styleSheet);
    seasonLabel->setText(clockstrings::seasonText(season));
}

void MumeClockWidget::updateWeather(PromptWeatherEnum weather)
{
    const QString text = clockstrings::weatherEmoji(weather);
    weatherLabel->setText(text);
    weatherLabel->setStatusTip(clockstrings::weatherTooltip(weather));
    weatherLabel->setVisible(!text.isEmpty());
}

void MumeClockWidget::updateFog(PromptFogEnum fog)
{
    const QString text = clockstrings::fogEmoji(fog);
    fogLabel->setText(text);
    fogLabel->setStatusTip(clockstrings::fogTooltip(fog));
    fogLabel->setVisible(!text.isEmpty());
}

void MumeClockWidget::updateCountdown(const MumeMoment &moment)
{
    // FIXME: Use ChangeMonitor
    setVisible(getConfig().mumeClock.display);
    if (!getConfig().mumeClock.display) {
        return;
    }

    const MumeClockPrecisionEnum precision = m_clock.getPrecision();
    if (precision <= MumeClockPrecisionEnum::HOUR) {
        timeLabel->setText(QString::fromUtf8("\xE2\x9A\xA0").append(m_clock.toCountdown(moment)));
    } else {
        timeLabel->setText(m_clock.toCountdown(moment));
    }
}

void MumeClockWidget::updateStatusTips(const MumeMoment &moment)
{
    moonPhaseLabel->setStatusTip(moment.toMumeMoonTime());
    seasonLabel->setStatusTip(m_clock.toMumeTime(moment));

    QString statusTip = "";
    const MumeClockPrecisionEnum precision = m_clock.getPrecision();
    const auto time = moment.toTimeOfDay();
    if (time == MumeTimeEnum::DAWN) {
        statusTip = "Ticks left until day";
    } else if (time >= MumeTimeEnum::DUSK) {
        statusTip = "Ticks left until day";
    } else {
        statusTip = "Ticks left until night";
    }
    if (precision != MumeClockPrecisionEnum::MINUTE) {
        statusTip = "The clock has not synced with MUME! Click to override at your own risk.";
    }
    timeLabel->setStatusTip(statusTip);
}
