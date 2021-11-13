// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeclock.h"

#include <cassert>
#include <QHash>
#include <QMetaEnum>
#include <QObject>
#include <QRegularExpression>
#include <QString>

#include "../global/Array.h"
#include "mumemoment.h"

static constexpr const int DEFAULT_MUME_START_EPOCH = 1517443173;
static constexpr const int DEFAULT_TOLERANCE_LIMIT = 10;
static constexpr const int ONE_RL_DAY_IN_SECONDS = 86400;

NODISCARD static inline int am(int h)
{
    return h;
}
NODISCARD static inline int pm(int h)
{
    return h + 12;
}

static const MMapper::Array<int, 12> s_dawnHour
    = {am(8), am(9), am(8), am(7), am(7), am(6), am(5), am(4), am(5), am(6), am(7), am(7)};
static const MMapper::Array<int, 12> s_duskHour
    = {pm(6), pm(5), pm(6), pm(7), pm(8), pm(8), pm(9), pm(10), pm(9), pm(8), pm(8), pm(7)};

const QMetaEnum MumeClock::s_westronMonthNames
    = QMetaEnum::fromType<MumeClock::WestronMonthNamesEnum>();
const QMetaEnum MumeClock::s_sindarinMonthNames
    = QMetaEnum::fromType<MumeClock::SindarinMonthNamesEnum>();
const QMetaEnum MumeClock::s_westronWeekDayNames
    = QMetaEnum::fromType<MumeClock::WestronWeekDayNamesEnum>();
const QMetaEnum MumeClock::s_sindarinWeekDayNames
    = QMetaEnum::fromType<MumeClock::SindarinWeekDayNamesEnum>();

const QHash<QString, MumeTimeEnum> MumeClock::m_stringTimeHash{
    // Generic Outdoors
    {"The day has begun.", MumeTimeEnum::DAY},
    {"The day has begun. You feel so weak under the cruel light!", MumeTimeEnum::DAY},
    {"The night has begun.", MumeTimeEnum::NIGHT},
    {"The night has begun. You feel stronger in the dark!", MumeTimeEnum::NIGHT},

    // Generic Indoors
    {"Light gradually filters in, proclaiming a new sunrise outside.", MumeTimeEnum::DAWN},
    {"It seems as if the day has begun.", MumeTimeEnum::DAY},
    {"The deepening gloom announces another sunset outside.", MumeTimeEnum::DUSK},
    {"It seems as if the night has begun.", MumeTimeEnum::NIGHT},
    {"The last ray of light fades, and all is swallowed up in darkness.", MumeTimeEnum::NIGHT},

    // Necromancer Darkness
    {"Arda seems to wither as an evil power begins to grow...", MumeTimeEnum::UNKNOWN},
    {"Shrouds of dark clouds roll in above you, blotting out the skies.", MumeTimeEnum::UNKNOWN},
    {"The evil power begins to regress...", MumeTimeEnum::UNKNOWN},

    // Bree
    {"The sun rises slowly above Bree Hill.", MumeTimeEnum::DAWN},
    {"The sun sets on Bree-land.", MumeTimeEnum::DUSK},

    // GH
    {"The sun rises over the city wall.", MumeTimeEnum::DAWN},
    {"Rays of sunshine pierce the darkness as the sun begins its gradual ascent to the middle of the sky.",
     MumeTimeEnum::DAWN},

    // Warrens
    {"The sun sinks slowly below the western horizon.", MumeTimeEnum::DUSK},

    // Fornost
    {"The sun rises in the east.", MumeTimeEnum::DAWN},
    {"The sun slowly rises over the rooftops in the east.", MumeTimeEnum::DAWN},
    {"The sun slowly disappears in the west.", MumeTimeEnum::DUSK},
};

MumeClock::MumeClock(int64_t mumeEpoch, QObject *parent)
    : QObject(parent)
    , m_mumeStartEpoch(mumeEpoch)
    , m_precision(MumeClockPrecisionEnum::UNSET)
    , m_clockTolerance(DEFAULT_TOLERANCE_LIMIT)
{}

MumeClock::MumeClock()
    : MumeClock(DEFAULT_MUME_START_EPOCH, nullptr)
{}

MumeMoment MumeClock::getMumeMoment() const
{
    const int64_t t = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    return MumeMoment::sinceMumeEpoch(t - m_mumeStartEpoch);
}

MumeMoment MumeClock::getMumeMoment(const int64_t secsSinceUnixEpoch) const
{
    /* This will break on 2038-01-19 if you use 32-bit. */
    if (secsSinceUnixEpoch < 0) {
        assert(secsSinceUnixEpoch == -1);
        return getMumeMoment();
    }
    return MumeMoment::sinceMumeEpoch(secsSinceUnixEpoch - m_mumeStartEpoch);
}

void MumeClock::parseMumeTime(const QString &mumeTime)
{
    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    parseMumeTime(mumeTime, secsSinceEpoch);
}

void MumeClock::parseMumeTime(const QString &mumeTime, const int64_t secsSinceEpoch)
{
    const auto moment = MumeMoment::sinceMumeEpoch(secsSinceEpoch - m_mumeStartEpoch);
    int minute = moment.minute;
    int hour = moment.hour;
    int day = 0;
    int month = 0;
    int year = MUME_START_YEAR;
    int weekDay = -1;

    if (mumeTime.at(0).isDigit()) {
        // 3 pm on Highday, the 18th of Halimath, year 3030 of the Third Age.
        static const QRegularExpression rx(
            R"(^(\d+)(?::\d{2})?\W*(am|pm) on (\w+), the (\d+).{2} of (\w+), year (\d+) of the Third Age.$)");
        auto match = rx.match(mumeTime);
        if (!match.hasMatch())
            return;
        hour = match.captured(1).toInt();
        if (match.captured(2).at(0) == 'p') {
            // pm
            if (hour != 12) {
                // add 12 if not noon
                hour += 12;
            }
        } else if (hour == 12) {
            // midnight
            hour = 0;
        }
        weekDay = s_westronWeekDayNames.keyToValue(match.captured(3).toLatin1().data());
        if (weekDay == static_cast<int>(WestronWeekDayNamesEnum::UnknownWestronWeekDay)) {
            weekDay = s_sindarinWeekDayNames.keysToValue(match.captured(3).toLatin1().data());
        }
        day = match.captured(4).toInt() - 1;
        month = s_westronMonthNames.keyToValue(match.captured(5).toLatin1().data());
        if (month == static_cast<int>(WestronMonthNamesEnum::UnknownWestronMonth)) {
            month = s_sindarinMonthNames.keyToValue(match.captured(5).toLatin1().data());
        }
        year = match.captured(6).toInt();
        if (m_precision <= MumeClockPrecisionEnum::DAY) {
            m_precision = MumeClockPrecisionEnum::HOUR;
        }

    } else {
        // "Highday, the 18th of Halimath, year 3030 of the Third Age."
        static const QRegularExpression rx(
            R"(^(\w+), the (\d+).{2} of (\w+), year (\d+) of the Third Age.$)");
        auto match = rx.match(mumeTime);
        if (!match.hasMatch())
            return;
        weekDay = s_westronWeekDayNames.keyToValue(match.captured(1).toLatin1().data());
        if (weekDay == static_cast<int>(WestronWeekDayNamesEnum::UnknownWestronWeekDay)) {
            weekDay = s_sindarinWeekDayNames.keysToValue(match.captured(1).toLatin1().data());
        }
        day = match.captured(2).toInt() - 1;
        month = s_westronMonthNames.keyToValue(match.captured(3).toLatin1().data());
        if (month == static_cast<int>(WestronMonthNamesEnum::UnknownWestronMonth)) {
            month = s_sindarinMonthNames.keyToValue(match.captured(3).toLatin1().data());
        }
        year = match.captured(4).toInt();
        if (m_precision <= MumeClockPrecisionEnum::UNSET) {
            m_precision = MumeClockPrecisionEnum::DAY;
        }
    }

    // Update last sync timestamp
    m_lastSyncEpoch = secsSinceEpoch;

    // Calculate start of Mume epoch
    auto capturedMoment = MumeMoment(year, month, day, hour, minute);
    const int mumeSecsSinceEpoch = capturedMoment.toSeconds();
    const int64_t newStartEpoch = secsSinceEpoch - mumeSecsSinceEpoch;
    if (newStartEpoch != m_mumeStartEpoch) {
        log("Detected new Mume start epoch " + QString::number(newStartEpoch) + " ("
            + QString::number(newStartEpoch - m_mumeStartEpoch) + " seconds from previous)");
    } else {
        log("Synchronized clock using 'time' output.");
    }
    if (weekDay != capturedMoment.weekDay()) {
        qWarning() << "Calculated week day does not match MUME";
    }
    m_mumeStartEpoch = newStartEpoch;
}

void MumeClock::parseWeather(const QString &str)
{
    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    parseWeather(str, secsSinceEpoch);
}

void MumeClock::parseWeather(const QString &str, int64_t secsSinceEpoch)
{
    if (!m_stringTimeHash.contains(str)) {
        return;
    }

    // Update last sync timestamp
    m_lastSyncEpoch = secsSinceEpoch;

    const MumeTimeEnum time = m_stringTimeHash.value(str);
    auto moment = MumeMoment::sinceMumeEpoch(secsSinceEpoch - m_mumeStartEpoch);

    // Predict current hour given the month
    const auto dawnDusk = getDawnDusk(moment.month);
    const int dawn = dawnDusk.dawnHour;
    const int dusk = dawnDusk.duskHour;
    switch (time) {
    case MumeTimeEnum::DAWN:
        moment.hour = dawn;
        break;
    case MumeTimeEnum::DAY:
        moment.hour = dawn + 1;
        break;
    case MumeTimeEnum::DUSK:
        moment.hour = dusk;
        break;
    case MumeTimeEnum::NIGHT:
        moment.hour = dusk + 1;
        break;
    case MumeTimeEnum::UNKNOWN:
        unknownTimeTick(moment);
        break;
    default:
        return;
    }
    // Set minute to zero
    moment.minute = 0;

    // Update epoch
    m_precision = MumeClockPrecisionEnum::MINUTE;
    m_mumeStartEpoch = secsSinceEpoch - moment.toSeconds();
    log("Synchronized tick using weather");
}

MumeMoment &MumeClock::unknownTimeTick(MumeMoment &moment)
{
    // Sync
    if (m_precision == MumeClockPrecisionEnum::HOUR) {
        // Assume we are moving forward in time
        moment.hour = moment.hour + 1;
        m_precision = MumeClockPrecisionEnum::MINUTE;
        log("Synchronized tick and raised precision");
    } else {
        if (moment.minute == 0) {
            m_precision = MumeClockPrecisionEnum::MINUTE;
            log("Tick detected");
        } else {
            if (moment.minute > 0 && moment.minute <= m_clockTolerance) {
                log("Synchronized tick but Mume seems to be running slow by "
                    + QString::number(moment.minute) + " seconds");
            } else if (moment.minute >= (60 - m_clockTolerance) && moment.minute < 60) {
                log("Synchronized tick but Mume seems to be running fast by "
                    + QString::number(moment.minute) + " seconds");
                moment.hour = moment.hour + 1;
            } else {
                m_precision = MumeClockPrecisionEnum::DAY;
                log("Precision lowered because tick was off by " + QString::number(moment.minute)
                    + " seconds)");
            }
        }
    }
    return moment;
}

void MumeClock::parseClockTime(const QString &clockTime)
{
    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    parseClockTime(clockTime, secsSinceEpoch);
}

void MumeClock::parseClockTime(const QString &clockTime, const int64_t secsSinceEpoch)
{
    // The current time is 5:23pm.
    static const QRegularExpression rx(R"(^The current time is (\d+):(\d+)\W*(am|pm).$)");
    auto match = rx.match(clockTime);
    if (!match.hasMatch())
        return;

    int hour = match.captured(1).toInt();
    int minute = match.captured(2).toInt();
    if (match.captured(3).at(0) == 'p') {
        // pm
        if (hour != 12) {
            // add 12 if not noon
            hour += 12;
        }
    } else if (hour == 12) {
        // midnight
        hour = 0;
    }

    m_precision = MumeClockPrecisionEnum::MINUTE;
    auto moment = MumeMoment::sinceMumeEpoch(secsSinceEpoch - m_mumeStartEpoch);
    moment.minute = minute;
    moment.hour = hour;
    const int64_t newStartEpoch = secsSinceEpoch - moment.toSeconds();
    log("Synchronized with clock in room (" + QString::number(newStartEpoch - m_mumeStartEpoch)
        + " seconds from previous)");
    m_mumeStartEpoch = newStartEpoch;
}

// TODO: move this somewhere useful?
NODISCARD static const char *getOrdinalSuffix(const int day)
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

void MumeClock::setPrecision(const MumeClockPrecisionEnum precision)
{
    m_precision = precision;
}

MumeClockPrecisionEnum MumeClock::getPrecision()
{
    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    if (m_precision >= MumeClockPrecisionEnum::HOUR
        && secsSinceEpoch - m_lastSyncEpoch > ONE_RL_DAY_IN_SECONDS) {
        m_precision = MumeClockPrecisionEnum::DAY;
        log("Precision lowered because clock has not been synced recently.");
    }
    return m_precision;
}

QString MumeClock::toMumeTime(const MumeMoment &moment) const
{
    int hour = moment.hour;
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

    QString weekDay = MumeClock::s_westronWeekDayNames.valueToKey(moment.weekDay());
    QString time;
    switch (m_precision) {
    case MumeClockPrecisionEnum::HOUR:
        time = QString("%1%2 on %3").arg(hour).arg(period).arg(weekDay);
        break;
    case MumeClockPrecisionEnum::MINUTE:
        time = QString("%1:%2%3 on %4")
                   .arg(hour)
                   .arg(moment.minute, 2, 10, QChar('0'))
                   .arg(period)
                   .arg(weekDay);
        break;
    case MumeClockPrecisionEnum::UNSET:
    case MumeClockPrecisionEnum::DAY:
    default:
        time = weekDay;
        break;
    }

    const int day = moment.day + 1;
    // TODO: Detect what calendar the player is using
    QString monthName = MumeClock::s_westronMonthNames.valueToKey(moment.month);
    return QString("%1, the %2%3 of %4, year %5 of the Third Age.")
        .arg(time)
        .arg(day)
        .arg(QString{getOrdinalSuffix(day)})
        .arg(monthName)
        .arg(moment.year);
}

QString MumeClock::toCountdown(const MumeMoment &moment) const
{
    auto dawnDusk = getDawnDusk(moment.month);
    const int dawn = dawnDusk.dawnHour;
    const int dusk = dawnDusk.duskHour;

    // Add seconds until night or day
    int secondsToCountdown = (m_precision == MumeClockPrecisionEnum::MINUTE) ? 60 - moment.minute
                                                                             : 0;
    if (moment.hour <= dawn) {
        // Add seconds until dawn
        secondsToCountdown += (dawn - moment.hour) * 60;
    } else if (moment.hour >= dusk) {
        // Add seconds until dawn
        secondsToCountdown += (24 - moment.hour + dawn) * 60;
    } else {
        // Add seconds until dusk
        secondsToCountdown += (dusk - 1 - moment.hour) * 60;
    }
    if (m_precision <= MumeClockPrecisionEnum::HOUR) {
        return QString("~%1").arg((secondsToCountdown / 60) + 1);
    }
    return QString("%1:%2")
        .arg(secondsToCountdown / 60)
        .arg(secondsToCountdown % 60, 2, 10, QChar('0'));
}

MumeClock::DawnDusk MumeClock::getDawnDusk(const int month)
{
    assert(month >= 0 && month < NUM_MONTHS);
    const auto m = static_cast<uint32_t>(month);
    return DawnDusk{s_dawnHour.at(m), s_duskHour.at(m)};
}
