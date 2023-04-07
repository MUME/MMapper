// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeclock.h"

#include <cassert>
#include <QMetaEnum>
#include <QObject>
#include <QRegularExpression>
#include <QString>

#include "../global/Array.h"
#include "../proxy/GmcpMessage.h"
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

MumeClock::MumeClock(int64_t mumeEpoch, QObject *const parent)
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
    setLastSyncEpoch(secsSinceEpoch);

    // Calculate start of Mume epoch
    auto capturedMoment = MumeMoment(year, month, day, hour, minute);
    const int mumeSecsSinceEpoch = capturedMoment.toSeconds();
    const int64_t newStartEpoch = secsSinceEpoch - mumeSecsSinceEpoch;
    if (newStartEpoch != m_mumeStartEpoch) {
        log("Detected new Mume start epoch " + QString::number(newStartEpoch) + " ("
            + QString::number(newStartEpoch - m_mumeStartEpoch) + " seconds from previous)");
    } else {
        log("Synchronized clock using 'time' output");
    }
    if (weekDay != capturedMoment.weekDay()) {
        qWarning() << "Calculated week day does not match MUME";
    }
    m_mumeStartEpoch = newStartEpoch;
}

void MumeClock::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (!(msg.isEventMoon() || msg.isEventDarkness() || msg.isEventSun())
        || !msg.getJsonDocument().has_value() || !msg.getJsonDocument()->isObject())
        return;

    const QJsonObject obj = msg.getJsonDocument()->object();
    if (!obj.contains("what") || !obj.value("what").isString())
        return;

    const auto type = obj["what"].toString();
    if (type.isEmpty())
        return;

    MumeTimeEnum time = MumeTimeEnum::UNKNOWN;
    if (msg.isEventSun()) {
        switch (type.at(0).toLatin1()) {
        case 'l': // light
            time = MumeTimeEnum::DAY;
            break;
        case 'd': // dark
            time = MumeTimeEnum::NIGHT;
            break;
        case 'r': // rise
            time = MumeTimeEnum::DAWN;
            break;
        case 's': // set
            time = MumeTimeEnum::DUSK;
            break;
        default:
            assert(false);
            break;
        }
    }
    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    parseWeather(time, secsSinceEpoch);
}

void MumeClock::parseWeather(const MumeTimeEnum time, int64_t secsSinceEpoch)
{
    // Update last sync timestamp
    setLastSyncEpoch(secsSinceEpoch);

    // Set minute to zero
    auto moment = MumeMoment::sinceMumeEpoch(secsSinceEpoch - m_mumeStartEpoch);
    moment.minute = 0;

    // Predict current hour given the month
    const auto dawnDusk = getDawnDusk(moment.month);
    const int dawn = dawnDusk.dawnHour;
    const int dusk = dawnDusk.duskHour;

    const char *type;
    switch (time) {
    case MumeTimeEnum::DAWN:
        moment.hour = dawn;
        type = "sunrise";
        break;
    case MumeTimeEnum::DAY:
        moment.hour = dawn + 1;
        type = "day";
        break;
    case MumeTimeEnum::DUSK:
        moment.hour = dusk;
        type = "sunset";
        break;
    case MumeTimeEnum::NIGHT:
        moment.hour = dusk + 1;
        type = "night";
        break;
    case MumeTimeEnum::UNKNOWN:
        type = "weather";
        break;
    }

    // Update epoch
    m_mumeStartEpoch = secsSinceEpoch - moment.toSeconds();
    if (time == MumeTimeEnum::UNKNOWN && moment.minute != 0) {
        m_precision = MumeClockPrecisionEnum::DAY;
        log(QString("Unsychronized tick detected using %1 (off by %2 seconds)")
                .arg(type)
                .arg(moment.minute));
    } else {
        m_precision = MumeClockPrecisionEnum::MINUTE;
        log(QString("Synchronized tick using %1").arg(type));
    }
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
        log("Precision lowered because clock has not been synced recently");
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
