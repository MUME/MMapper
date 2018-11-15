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

#ifndef MUMECLOCK_H
#define MUMECLOCK_H

#include <array>
#include <QHash>
#include <QList>
#include <QMetaEnum>
#include <QObject>
#include <QString>
#include <QtCore>

#include "mumemoment.h"

class QMetaEnum;

enum class MumeClockPrecision {
    MUMECLOCK_UNSET = -1,
    MUMECLOCK_DAY,
    MUMECLOCK_HOUR,
    MUMECLOCK_MINUTE
};

class MumeClock final : public QObject
{
    Q_OBJECT

    friend class TestClock;

public:
    static constexpr const int NUM_MONTHS = 12;
    struct DawnDusk
    {
        int dawnHour;
        int duskHour;
    };
    static DawnDusk getDawnDusk(int month);

public:
    explicit MumeClock(int64_t mumeEpoch, QObject *parent = nullptr);

    explicit MumeClock(QObject *parent = nullptr);

    MumeMoment getMumeMoment();

    // TODO: #ifdef TEST?
    MumeMoment getMumeMoment(int64_t secsSinceUnixEpoch);

    MumeClockPrecision getPrecision() { return m_precision; }

    const QString toMumeTime(const MumeMoment &moment);

    const QString toCountdown(const MumeMoment &moment);

    int64_t getMumeStartEpoch() { return m_mumeStartEpoch; }

    enum class WestronMonthNames {
        UnknownWestronMonth = -1,
        Afteryule,
        Solmath,
        Rethe,
        Astron,
        Thrimidge,
        Forelithe,
        Afterlithe,
        Wedmath,
        Halimath,
        Winterfilth,
        Blotmath,
        Foreyule
    };

    Q_ENUM(WestronMonthNames)

    enum class SindarinMonthNames {
        UnknownSindarinMonth = -1,
        Narwain,
        Ninui,
        Gwaeron,
        Gwirith,
        Lothron,
        Norui,
        Cerveth,
        Urui,
        Ivanneth,
        Narbeleth,
        Hithui,
        Girithron
    };

    Q_ENUM(SindarinMonthNames)

private:
    static const std::array<int, NUM_MONTHS> s_dawnHour;
    static const std::array<int, NUM_MONTHS> s_duskHour;

public:
    static const QMetaEnum s_westronMonthNames;
    static const QMetaEnum s_sindarinMonthNames;
    static const QHash<QString, MumeTime> m_stringTimeHash;

signals:

    void log(const QString &, const QString &);

public slots:

    void parseMumeTime(const QString &mumeTime);

    void parseClockTime(const QString &clockTime);

    void parseWeather(const QString &str);

protected:
    void setPrecision(MumeClockPrecision state) { m_precision = state; }

    void parseMumeTime(const QString &mumeTime, int64_t secsSinceEpoch);

    void parseClockTime(const QString &clockTime, int64_t secsSinceEpoch);

    void parseWeather(const QString &str, int64_t secsSinceEpoch);

private:
    MumeMoment &unknownTimeTick(MumeMoment &moment);

    int64_t m_mumeStartEpoch = 0;
    MumeClockPrecision m_precision{};
    int m_clockTolerance = 0;
};

#endif // MUMECLOCK_H
