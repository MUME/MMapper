#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Signal2.h"
#include "../observer/gameobserver.h"
#include "mumeclock.h"
#include "mumemoment.h"
#include "ui_mumeclockwidget.h"

#include <QString>
#include <QWidget>
#include <QtCore>

class QEvent;
class QMouseEvent;

class NODISCARD_QOBJECT MumeClockWidget final : public QWidget, private Ui::MumeClockWidget
{
    Q_OBJECT

private:
    Signal2Lifetime m_lifetime;
    MumeClock &m_clock;

public:
    explicit MumeClockWidget(GameObserver &observer, MumeClock &clock, QWidget *parent);
    ~MumeClockWidget() final;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

private:
    void updateCountdown(const MumeMoment &moment);
    void updateTime(MumeTimeEnum time);
    void updateMoonPhase(MumeMoonPhaseEnum phase);
    void updateMoonVisibility(MumeMoonVisibilityEnum visibility);
    void updateSeason(MumeSeasonEnum season);
    void updateStatusTips(const MumeMoment &moment);
};
