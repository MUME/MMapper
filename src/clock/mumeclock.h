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

enum MumeClockPrecision { MUMECLOCK_UNSET = -1, MUMECLOCK_DAY, MUMECLOCK_HOUR, MUMECLOCK_MINUTE };

#include <QObject>
#include <QList>
#include <QMetaEnum>
#include <QHash>

#include "mumemoment.h"

class MumeClock : public QObject
{
    Q_OBJECT
    friend class TestClock;

public:
    MumeClock(int mumeEpoch, QObject *parent = 0);
    MumeClock(QObject *parent = 0);

    MumeMoment getMumeMoment(int secsSinceUnixEpoch = -1);
    MumeClockPrecision getPrecision()
    {
        return m_precision;
    }
    const QString toMumeTime(const MumeMoment &moment);
    const QString toCountdown(const MumeMoment &moment);
    int getMumeStartEpoch()
    {
        return m_mumeStartEpoch;
    }

    enum WestronMonthNames { UnknownWestronMonth = -1, Afteryule, Solmath, Rethe, Astron, Thrimidge, Forelithe, Afterlithe, Wedmath, Halimath, Winterfilth, Blotmath, Foreyule };
    Q_ENUM(WestronMonthNames)

    enum SindarinMonthNames { UnknownSindarinMonth = -1, Narwain, Ninui, Gwaeron, Gwirith, Lothron, Norui, Cerveth, Urui, Ivanneth, Narbeleth, Hithui, Girithron };
    Q_ENUM(SindarinMonthNames)

    static const QList<int> m_dawnHour;
    static const QList<int> m_duskHour;
    static const QMetaEnum m_westronMonthNames;
    static const QMetaEnum m_sindarinMonthNames;
    static const QHash<QString, MumeTime> m_stringTimeHash;

signals:
    void log(const QString &, const QString &);

public slots:
    void parseMumeTime(const QString &mumeTime);
    void parseClockTime(const QString &clockTime);
    void parseWeather(const QString &str);

protected:
    void setPrecision(MumeClockPrecision state)
    {
        m_precision = state;
    }
    void parseMumeTime(const QString &mumeString, int secsSinceUnixEpoch);
    void parseClockTime(const QString &clockTime, int secsSinceUnixEpoch);
    void parseWeather(const QString &str, int secsSinceEpoch);

private:
    MumeMoment &unknownTimeTick(MumeMoment &time);

    int m_mumeStartEpoch;
    int m_clockTolerance;
    MumeClockPrecision m_precision;
};

#endif // MUMECLOCK_H
