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

#include <QRegExp>
#include <QDateTime>
#include <QDebug>

#include "mumeclock.h"
#include "mumemoment.h"

#define DEFAULT_MUME_START_EPOCH 1420070400
#define DEFAULT_TOLERANCE_LIMIT 10

QList<int> MumeClock::m_dawnHour = QList<int>() << 8 << 9 << 8 << 7 << 7 << 6 << 5 << 4 << 5 << 6 << 7 << 7;
QList<int> MumeClock::m_duskHour = QList<int>() << 6+12 << 5+12 << 6+12 << 7+12 << 8+12 << 8+12 << 9+12 << 10+12 << 9+12 << 8+12 << 8+12 << 7+12;
QMetaEnum MumeClock::m_westronMonthNames = QMetaEnum::fromType<MumeClock::WestronMonthNames>();
QMetaEnum MumeClock::m_sindarinMonthNames = QMetaEnum::fromType<MumeClock::SindarinMonthNames>();

MumeClock::MumeClock(int mumeEpoch, QObject *parent)
    : QObject(parent),
      m_mumeStartEpoch(mumeEpoch),
      m_precision(MUMECLOCK_UNSET),
      m_clockTolerance(DEFAULT_TOLERANCE_LIMIT)
{}


MumeClock::MumeClock(QObject *parent)
    : MumeClock(DEFAULT_MUME_START_EPOCH, parent)
{}

MumeMoment MumeClock::getMumeMoment(int secsSinceUnixEpoch)
{
    if (secsSinceUnixEpoch < 0)
        secsSinceUnixEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    return MumeMoment(secsSinceUnixEpoch - m_mumeStartEpoch);
}

void MumeClock::parseMumeTime(const QString& mumeTime)
{
    int secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    parseMumeTime(mumeTime, secsSinceEpoch);
}

void MumeClock::parseMumeTime(const QString& mumeTime, int secsSinceEpoch)
{
    MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
    int minute = moment.m_minute;
    int hour = moment.m_hour;
    int day = 0;
    int month = 0;
    int year = MUME_START_YEAR;

    if (mumeTime.at(0).isDigit())
    {
        // 3pm on Highday, the 18th of Halimath, Year 3030 of the Third Age.
        QRegExp rx("(\\d+)(am|pm) on \\w+, the (\\d+).{2} of (\\w+), Year (\\d+) of the Third Age.");
        if (rx.indexIn(mumeTime) != -1) {
            hour = rx.cap(1).toInt();
            if (rx.cap(2).at(0) == 'p')
            {   // pm
                if (hour != 12)
                {   // add 12 if not noon
                    hour += 12;
                }
            }
            else if (hour == 12)
            {   // midnight
                hour = 0;
            }
            day = rx.cap(3).toInt() - 1;
            month = m_westronMonthNames.keyToValue(rx.cap(4).toLatin1().data());
            if (month == UnknownWestronMonth)
            {
                month = m_sindarinMonthNames.keyToValue(rx.cap(4).toLatin1().data());
            }
            year = rx.cap(5).toInt();
            if (m_precision <= MUMECLOCK_DAY)
                m_precision = MUMECLOCK_HOUR;
        }
    }
    else
    {
        // "Highday, the 18th of Halimath, Year 3030 of the Third Age."
        QRegExp rx("\\w+, the (\\d+).{2} of (\\w+), Year (\\d+) of the Third Age.");
        if (rx.indexIn(mumeTime) != -1) {
            day = rx.cap(1).toInt() - 1;
            month = m_westronMonthNames.keyToValue(rx.cap(2).toLatin1().data());
            if (month == UnknownWestronMonth)
            {
                month = m_sindarinMonthNames.keyToValue(rx.cap(2).toLatin1().data());
            }
            year = rx.cap(3).toInt();
            if (m_precision <= MUMECLOCK_UNSET)
                m_precision = MUMECLOCK_DAY;
        }
    }

    // Calculate start of Mume epoch
    int mumeSecsSinceEpoch = MumeMoment(year, month, day, hour, minute).toSeconds();
    int newStartEpoch = secsSinceEpoch - mumeSecsSinceEpoch;
    if (newStartEpoch != m_mumeStartEpoch)
        emit log("MumeClock", "Detected new Mume start epoch " + QString::number(newStartEpoch) + " (" + QString::number(newStartEpoch - m_mumeStartEpoch) + " seconds from previous)");
    m_mumeStartEpoch = newStartEpoch;
}

void MumeClock::tickSync(MumeTime time)
{
    int secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    if (time == TIME_UNKNOWN)
        return tickSync(secsSinceEpoch);

    MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
    moment.m_minute = 0;

    // Predict current hour given the month
    int dawn = m_dawnHour[moment.m_month];
    int dusk = m_duskHour[moment.m_month];
    switch (time)
    {
    case TIME_DAWN: moment.m_hour=dawn;break;
    case TIME_DAY: moment.m_hour=dawn+1;break;
    case TIME_DUSK: moment.m_hour=dusk;break;
    case TIME_NIGHT: moment.m_hour=dusk+1;break;
    }

    // Update epoch
    m_precision = MUMECLOCK_MINUTE;
    m_mumeStartEpoch = secsSinceEpoch - moment.toSeconds();
    emit log("MumeClock", "Synchronized tick using weather");
}

void MumeClock::tickSync(int secsSinceEpoch)
{
    MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
    // Only sync on ticks if we know the time with some reliability
    if (m_precision <= MUMECLOCK_DAY && moment.m_minute != 0)
    {
        emit log("MumeClock", "Tick detected but discarded until the hour is recognized");
        return ;
    }

    // Sync
    if (m_precision == MUMECLOCK_HOUR)
    {
        // Assume we are moving forward in time
        moment.m_hour = moment.m_hour + 1;
        moment.m_minute = 0;
        m_precision = MUMECLOCK_MINUTE;
        emit log("MumeClock", "Synchronized tick and raised precision");
    }
    else
    {
        if (moment.m_minute == 0)
        {
            m_precision = MUMECLOCK_MINUTE;
            emit log("MumeClock", "Tick detected");
            return ;
        }
        else
        {
            if (moment.m_minute > 0 && moment.m_minute <= m_clockTolerance)
            {
                emit log("MumeClock", "Synchronized tick but Mume seems to be running slow by " + QString::number(moment.m_minute) + " seconds");
                moment.m_minute = 0;
            }
            else if (moment.m_minute >= (60 - m_clockTolerance) && moment.m_minute < 60)
            {
                emit log("MumeClock", "Synchronized tick but Mume seems to be running fast by " + QString::number(moment.m_minute) + " seconds");
                moment.m_hour = moment.m_hour + 1;
                moment.m_minute = 0;
            }
            else
            {
                m_precision = MUMECLOCK_DAY;
                emit log("MumeClock", "Precision lowered because tick was off by " + QString::number(moment.m_minute) + " seconds)");
                return ;
            }

        }
    }
    // Update epoch
    m_mumeStartEpoch = secsSinceEpoch - moment.toSeconds();
}

void MumeClock::parseClockTime(const QString& clockTime)
{
    int secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    parseClockTime(clockTime, secsSinceEpoch);
}

void MumeClock::parseClockTime(const QString& clockTime, int secsSinceEpoch)
{
    // The current time is 5:23 pm.
    QRegExp rx("The current time is (\\d+):(\\d+) (am|pm).");
    if (rx.indexIn(clockTime) != -1) {
        int hour = rx.cap(1).toInt();
        int minute = rx.cap(2).toInt();
        if (rx.cap(3).at(0) == 'p')
        {   // pm
            if (hour != 12)
            {   // add 12 if not noon
                hour += 12;
            }
        }
        else if (hour == 12)
        {   // midnight
            hour = 0;
        }

        m_precision = MUMECLOCK_MINUTE;
        MumeMoment moment(secsSinceEpoch - m_mumeStartEpoch);
        moment.m_minute = minute;
        moment.m_hour = hour;
        int newStartEpoch = secsSinceEpoch - moment.toSeconds();
        emit log("MumeClock", "Synchronized with clock in room (" + QString::number(newStartEpoch - m_mumeStartEpoch) + " seconds from previous)");
        m_mumeStartEpoch = newStartEpoch;
    }
}

const QString MumeClock::toMumeTime(const MumeMoment &moment)
{
    int hour = moment.m_hour;
    QString period;
    if (hour == 0)
    {
        hour = 12;
        period = "am";
    }
    else if (hour == 12)
    {
        period = "pm";
    }
    else if (hour > 12)
    {
        hour -= 12;
        period = "pm";
    }
    else
    {
        period = "am";
    }
    QString time;
    switch (m_precision)
    {
    case MUMECLOCK_HOUR:
        time = QString("%1%2 on the ").arg(hour).arg(period);
        break;
    case MUMECLOCK_MINUTE:
        time = QString("%1:%2%3 on the ").arg(hour).arg(QString().sprintf("%02d", moment.m_minute)).arg(period);
        break;
    }
    int day = moment.m_day + 1;
    QString daySuffix;
    if (day == 1 || day == 21)
        daySuffix = "st";
    else if (day == 2 || day == 22)
        daySuffix = "nd";
    else if (day == 3 || day == 23)
        daySuffix = "rd";
    else
        daySuffix = "th";

    // TODO: Figure out how to reverse engineer the day of the week
    QString monthName = MumeClock::m_westronMonthNames.valueToKey((WestronMonthNames)moment.m_month);
    return QString("%1%2%3 of %4, Year %5 of the Third Age.")
            .arg(time)
            .arg(day)
            .arg(daySuffix)
            .arg(monthName)
            .arg(moment.m_year);
}


const QString MumeClock::toCountdown(const MumeMoment &moment) {
    int dawn = m_dawnHour[moment.m_month];
    int dusk = m_duskHour[moment.m_month];

    // Add seconds until night or day
    int secondsToCountdown = (m_precision == MUMECLOCK_MINUTE) ? 60 - moment.m_minute : 0;
    if (moment.m_hour <= dawn)
    {   // Add seconds until dawn
        secondsToCountdown += (dawn - moment.m_hour) * 60;
    }
    else if (moment.m_hour >= dusk)
    {   // Add seconds until dawn
        secondsToCountdown += (24 - moment.m_hour + dawn) * 60;
    }
    else
    {   // Add seconds until dusk
        secondsToCountdown += (dusk - 1 - moment.m_hour) * 60;
    }
    if (m_precision <= MUMECLOCK_HOUR)
        return QString("~%1").arg((secondsToCountdown / 60) + 1);
    else
        return QString("%1:%2").arg(secondsToCountdown / 60).arg(QString().sprintf("%02d", secondsToCountdown % 60));
}
