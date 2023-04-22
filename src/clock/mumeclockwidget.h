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

class QMouseEvent;
class QObject;
class QTimer;

class MumeClockWidget final : public QWidget, private Ui::MumeClockWidget
{
    Q_OBJECT
public:
    explicit MumeClockWidget(MumeClock *clock, QWidget *parent);
    ~MumeClockWidget() final;

public slots:
    void slot_updateLabel();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    MumeClock *m_clock = nullptr;
    std::unique_ptr<QTimer> m_timer;

    MumeTimeEnum m_lastTime = MumeTimeEnum::UNKNOWN;
    MumeSeasonEnum m_lastSeason = MumeSeasonEnum::UNKNOWN;
    MumeMoonPhaseEnum m_lastPhase = MumeMoonPhaseEnum::UNKNOWN;
    MumeMoonPositionEnum m_lastVisibility = MumeMoonPositionEnum::UNKNOWN;
    MumeClockPrecisionEnum m_lastPrecision = MumeClockPrecisionEnum::UNSET;
};
