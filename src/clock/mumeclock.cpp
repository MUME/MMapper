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

#include "mumeclock.h"

#include <array>
#include <QHash>
#include <QMetaEnum>
#include <QObject>
#include <QString>

#include "mumemoment.h"

static constexpr const int DEFAULT_MUME_START_EPOCH = 1517443173;
static constexpr const int DEFAULT_TOLERANCE_LIMIT = 10;

static inline int am(int h)
{
    return h;
}
static inline int pm(int h)
{
    return h + 12;
}

const std::array<int, 12> MumeClock::s_dawnHour
    = {am(8), am(9), am(8), am(7), am(7), am(6), am(5), am(4), am(5), am(6), am(7), am(7)};
const std::array<int, 12> MumeClock::s_duskHour
    = {pm(6), pm(5), pm(6), pm(7), pm(8), pm(8), pm(9), pm(10), pm(9), pm(8), pm(8), pm(7)};

const QMetaEnum MumeClock::s_westronMonthNames = QMetaEnum::fromType<MumeClock::WestronMonthNames>();
const QMetaEnum MumeClock::s_sindarinMonthNames
    = QMetaEnum::fromType<MumeClock::SindarinMonthNames>();

const QHash<QString, MumeTime> MumeClock::m_stringTimeHash{
    // Generic Outdoors
    {"The day has begun.", MumeTime::TIME_DAY},
    {"The night has begun.", MumeTime::TIME_NIGHT},

    // Generic Indoors
    {"Light gradually filters in, proclaiming a new sunrise outside.", MumeTime::TIME_DAWN},
    {"It seems as if the day has begun.", MumeTime::TIME_DAY},
    {"The deepening gloom announces another sunset outside.", MumeTime::TIME_DUSK},
    {"It seems as if the night has begun.", MumeTime::TIME_NIGHT},
    {"The last ray of light fades, and all is swallowed up in darkness.", MumeTime::TIME_NIGHT},

    // Necromancer Darkness
    {"Arda seems to wither as an evil power begins to grow...", MumeTime::TIME_UNKNOWN},
    {"Shrouds of dark clouds roll in above you, blotting out the skies.", MumeTime::TIME_UNKNOWN},
    {"The evil power begins to regress...", MumeTime::TIME_UNKNOWN},

    // Bree
    {"The sun rises slowly above Bree Hill.", MumeTime::TIME_DAWN},
    {"The sun sets on Bree-land.", MumeTime::TIME_DUSK},

    // GH
    {"The sun rises over the city wall.", MumeTime::TIME_DAWN},
    {"Rays of sunshine pierce the darkness as the sun begins its gradual ascent to the middle of the sky.",
     MumeTime::TIME_DAWN},

    // Warrens
    {"The sun sinks slowly below the western horizon.", MumeTime::TIME_DUSK},

    // Fornost
    {"The sun rises in the east.", MumeTime::TIME_DAWN},
    {"The sun slowly rises over the rooftops in the east.", MumeTime::TIME_DAWN},
    {"The sun slowly disappears in the west.", MumeTime::TIME_DUSK},
};

MumeClock::MumeClock(int mumeEpoch, QObject *parent)
    : QObject(parent)
    , m_mumeStartEpoch(mumeEpoch)
    , m_precision(MumeClockPrecision::MUMECLOCK_UNSET)
    , m_clockTolerance(DEFAULT_TOLERANCE_LIMIT)
{}

MumeClock::MumeClock(QObject *parent)
    : MumeClock(DEFAULT_MUME_START_EPOCH, parent)
{}

MumeMoment MumeClock::getMumeMoment(int secsSinceUnixEpoch)
{
    if (secsSinceUnixEpoch < 0) {
        secsSinceUnixEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    }
    return MumeMoment{secsSinceUnixEpoch - m_mumeStartEpoch};
}

void MumeClock::parseMumeTime(const QString &mumeTime)
{
    int secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    parseMumeTime(mumeTime, secsSinceEpoch);
}

void MumeClock::parseMumeTime(const QString &mumeTime, int secsSinceEpoch)
{
    MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
    int minute = moment.m_minute;
    int hour = moment.m_hour;
    int day = 0;
    int month = 0;
    int year = MUME_START_YEAR;

    if (mumeTime.at(0).isDigit()) {
        // 3pm on Highday, the 18th of Halimath, year 3030 of the Third Age.
        QRegExp rx(R"((\d+)(am|pm) on \w+, the (\d+).{2} of (\w+), year (\d+) of the Third Age.)");
        if (rx.indexIn(mumeTime) != -1) {
            hour = rx.cap(1).toInt();
            if (rx.cap(2).at(0) == 'p') {
                // pm
                if (hour != 12) {
                    // add 12 if not noon
                    hour += 12;
                }
            } else if (hour == 12) {
                // midnight
                hour = 0;
            }
            day = rx.cap(3).toInt() - 1;
            month = s_westronMonthNames.keyToValue(rx.cap(4).toLatin1().data());
            if (month == static_cast<int>(WestronMonthNames::UnknownWestronMonth)) {
                month = s_sindarinMonthNames.keyToValue(rx.cap(4).toLatin1().data());
            }
            year = rx.cap(5).toInt();
            if (m_precision <= MumeClockPrecision::MUMECLOCK_DAY) {
                m_precision = MumeClockPrecision::MUMECLOCK_HOUR;
            }
        }
    } else {
        // "Highday, the 18th of Halimath, year 3030 of the Third Age."
        QRegExp rx(R"(\w+, the (\d+).{2} of (\w+), year (\d+) of the Third Age.)");
        if (rx.indexIn(mumeTime) != -1) {
            day = rx.cap(1).toInt() - 1;
            month = s_westronMonthNames.keyToValue(rx.cap(2).toLatin1().data());
            if (month == static_cast<int>(WestronMonthNames::UnknownWestronMonth)) {
                month = s_sindarinMonthNames.keyToValue(rx.cap(2).toLatin1().data());
            }
            year = rx.cap(3).toInt();
            if (m_precision <= MumeClockPrecision::MUMECLOCK_UNSET) {
                m_precision = MumeClockPrecision::MUMECLOCK_DAY;
            }
        }
    }

    // Calculate start of Mume epoch
    int mumeSecsSinceEpoch = MumeMoment(year, month, day, hour, minute).toSeconds();
    int newStartEpoch = secsSinceEpoch - mumeSecsSinceEpoch;
    if (newStartEpoch != m_mumeStartEpoch) {
        emit log("MumeClock",
                 "Detected new Mume start epoch " + QString::number(newStartEpoch) + " ("
                     + QString::number(newStartEpoch - m_mumeStartEpoch)
                     + " seconds from previous)");
    } else {
        emit log("MumeClock", "Synchronized clock using 'time' output.");
    }
    m_mumeStartEpoch = newStartEpoch;
}

void MumeClock::parseWeather(const QString &str)
{
    int secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    parseWeather(str, secsSinceEpoch);
}

void MumeClock::parseWeather(const QString &str, int secsSinceEpoch)
{
    if (!m_stringTimeHash.contains(str)) {
        return;
    }

    MumeTime time = m_stringTimeHash.value(str);

    MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
    // Predict current hour given the month
    const int dawn = s_dawnHour[moment.m_month];
    const int dusk = s_duskHour[moment.m_month];
    switch (time) {
    case MumeTime::TIME_DAWN:
        moment.m_hour = dawn;
        break;
    case MumeTime::TIME_DAY:
        moment.m_hour = dawn + 1;
        break;
    case MumeTime::TIME_DUSK:
        moment.m_hour = dusk;
        break;
    case MumeTime::TIME_NIGHT:
        moment.m_hour = dusk + 1;
        break;
    case MumeTime::TIME_UNKNOWN:
        unknownTimeTick(moment);
        break;
    default:
        return;
    }
    // Set minute to zero
    moment.m_minute = 0;

    // Update epoch
    m_precision = MumeClockPrecision::MUMECLOCK_MINUTE;
    m_mumeStartEpoch = secsSinceEpoch - moment.toSeconds();
    emit log("MumeClock", "Synchronized tick using weather");
}

MumeMoment &MumeClock::unknownTimeTick(MumeMoment &moment)
{
    // Sync
    if (m_precision == MumeClockPrecision::MUMECLOCK_HOUR) {
        // Assume we are moving forward in time
        moment.m_hour = moment.m_hour + 1;
        m_precision = MumeClockPrecision::MUMECLOCK_MINUTE;
        emit log("MumeClock", "Synchronized tick and raised precision");
    } else {
        if (moment.m_minute == 0) {
            m_precision = MumeClockPrecision::MUMECLOCK_MINUTE;
            emit log("MumeClock", "Tick detected");
        } else {
            if (moment.m_minute > 0 && moment.m_minute <= m_clockTolerance) {
                emit log("MumeClock",
                         "Synchronized tick but Mume seems to be running slow by "
                             + QString::number(moment.m_minute) + " seconds");
            } else if (moment.m_minute >= (60 - m_clockTolerance) && moment.m_minute < 60) {
                emit log("MumeClock",
                         "Synchronized tick but Mume seems to be running fast by "
                             + QString::number(moment.m_minute) + " seconds");
                moment.m_hour = moment.m_hour + 1;
            } else {
                m_precision = MumeClockPrecision::MUMECLOCK_DAY;
                emit log("MumeClock",
                         "Precision lowered because tick was off by "
                             + QString::number(moment.m_minute) + " seconds)");
            }
        }
    }
    return moment;
}

void MumeClock::parseClockTime(const QString &clockTime)
{
    const int secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    parseClockTime(clockTime, secsSinceEpoch);
}

void MumeClock::parseClockTime(const QString &clockTime, int secsSinceEpoch)
{
    // The current time is 5:23pm.
    QRegExp rx("The current time is (\\d+):(\\d+)(am|pm).");
    if (rx.indexIn(clockTime) != -1) {
        int hour = rx.cap(1).toInt();
        int minute = rx.cap(2).toInt();
        if (rx.cap(3).at(0) == 'p') {
            // pm
            if (hour != 12) {
                // add 12 if not noon
                hour += 12;
            }
        } else if (hour == 12) {
            // midnight
            hour = 0;
        }

        m_precision = MumeClockPrecision::MUMECLOCK_MINUTE;
        MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
        moment.m_minute = minute;
        moment.m_hour = hour;
        int newStartEpoch = secsSinceEpoch - moment.toSeconds();
        emit log("MumeClock",
                 "Synchronized with clock in room ("
                     + QString::number(newStartEpoch - m_mumeStartEpoch)
                     + " seconds from previous)");
        m_mumeStartEpoch = newStartEpoch;
    }
}

// TODO: move this somewhere useful?
static const char *getOrdinalSuffix(const int day)
{
    switch (day % 100) {
    case 11:
    case 12:
    case 13:
        return "th";
    default:
        break;
    }
    switch (day % 10) {
    case 1:
        return "st";
    case 2:
        return "nd";
    case 3:
        return "rd";
    default:
        return "th";
    }
}

const QString MumeClock::toMumeTime(const MumeMoment &moment)
{
    int hour = moment.m_hour;
    QString period;
    if (hour == 0) {
        hour = 12;
        period = "am";
    } else if (hour == 12) {
        period = "pm";
    } else if (hour > 12) {
        hour -= 12;
        period = "pm";
    } else {
        period = "am";
    }

    QString time;
    switch (m_precision) {
    case MumeClockPrecision::MUMECLOCK_HOUR:
        time = QString("%1%2 on the ").arg(hour).arg(period);
        break;
    case MumeClockPrecision::MUMECLOCK_MINUTE:
        time = QString("%1:%2%3 on the ")
                   .arg(hour)
                   .arg(QString().sprintf("%02d", moment.m_minute))
                   .arg(period);
        break;
    default:
        break;
    }

    const int day = moment.m_day + 1;

    // TODO(nschimme): Figure out how to reverse engineer the day of the week
    QString monthName = MumeClock::s_westronMonthNames.valueToKey(moment.m_month);
    return QString("%1%2%3 of %4, year %5 of the Third Age.")
        .arg(time)
        .arg(day)
        .arg(QString{getOrdinalSuffix(day)})
        .arg(monthName)
        .arg(moment.m_year);
}

const QString MumeClock::toCountdown(const MumeMoment &moment)
{
    const int dawn = s_dawnHour[moment.m_month];
    const int dusk = s_duskHour[moment.m_month];

    // Add seconds until night or day
    int secondsToCountdown = (m_precision == MumeClockPrecision::MUMECLOCK_MINUTE)
                                 ? 60 - moment.m_minute
                                 : 0;
    if (moment.m_hour <= dawn) {
        // Add seconds until dawn
        secondsToCountdown += (dawn - moment.m_hour) * 60;
    } else if (moment.m_hour >= dusk) {
        // Add seconds until dawn
        secondsToCountdown += (24 - moment.m_hour + dawn) * 60;
    } else {
        // Add seconds until dusk
        secondsToCountdown += (dusk - 1 - moment.m_hour) * 60;
    }
    if (m_precision <= MumeClockPrecision::MUMECLOCK_HOUR) {
        return QString("~%1").arg((secondsToCountdown / 60) + 1);
    }
    return QString("%1:%2")
        .arg(secondsToCountdown / 60)
        .arg(QString().sprintf("%02d", secondsToCountdown % 60));
}
