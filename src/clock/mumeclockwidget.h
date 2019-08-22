#pragma once
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
    QTimer *m_timer = nullptr;

    MumeTime m_lastTime = MumeTime::TIME_UNKNOWN;
    MumeSeason m_lastSeason = MumeSeason::SEASON_UNKNOWN;
    MumeMoonPhase m_lastPhase = MumeMoonPhase::PHASE_UNKNOWN;
    MumeMoonVisibility m_lastVisibility = MumeMoonVisibility::MOON_POSITION_UNKNOWN;
    MumeClockPrecision m_lastPrecision = MumeClockPrecision::MUMECLOCK_UNSET;
};
