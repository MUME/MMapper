#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QString>
#include <QWidget>
#include <QtCore>

#include "mumeclock.h"
#include "mumemoment.h"
#include "ui_mumeclockwidget.h"

class QObject;
class QTimer;

class MumeClockWidget : public QWidget, private Ui::MumeClockWidget
{
    Q_OBJECT
public:
    explicit MumeClockWidget(MumeClock *clock = nullptr, QWidget *parent = nullptr);
    ~MumeClockWidget();

public slots:
    void updateLabel();

private:
    MumeClock *m_clock = nullptr;
    std::unique_ptr<QTimer> m_timer;

    MumeTime m_lastTime = MumeTime::TIME_UNKNOWN;
    MumeSeason m_lastSeason = MumeSeason::SEASON_UNKNOWN;
    MumeMoonPhase m_lastPhase = MumeMoonPhase::PHASE_UNKNOWN;
    MumeMoonVisibility m_lastVisibility = MumeMoonVisibility::MOON_POSITION_UNKNOWN;
    MumeClockPrecision m_lastPrecision = MumeClockPrecision::MUMECLOCK_UNSET;
};
