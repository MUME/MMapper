// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeclock.h"

#include "../global/Array.h"
#include "../proxy/GmcpMessage.h"
#include "mumemoment.h"

#include <cassert>

#include <QDebug>
#include <QMetaEnum>
#include <QObject>
#include <QRegularExpression>
#include <QString>

namespace { // anonymous

constexpr const int DEFAULT_MUME_START_EPOCH = 1517443173;
constexpr const int ONE_RL_DAY_IN_SECONDS = 86400;

NODISCARD int am(const int h)
{
    assert(h >= 0 && h < 12);
    return h;
}
NODISCARD int pm(const int h)
{
    assert(h >= 0 && h < 12);
    return h + 12;
}

const MMapper::Array<int, 12> g_dawnHour
    = {am(8), am(9), am(8), am(7), am(7), am(6), am(5), am(4), am(5), am(6), am(7), am(7)};
const MMapper::Array<int, 12> g_duskHour
    = {pm(6), pm(5), pm(6), pm(7), pm(8), pm(8), pm(9), pm(10), pm(9), pm(8), pm(8), pm(7)};

// TODO: move this somewhere useful?
NODISCARD const char *getOrdinalSuffix(const int day)
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

} // namespace

namespace mmqt {
template<typename T>
class NODISCARD QME final
{
private:
    static_assert(std::is_enum_v<T>);
    using U = std::underlying_type_t<T>;
    static_assert(std::is_signed_v<U>);
    static_assert(static_cast<U>(T::Invalid) == -1);
    static inline const QMetaEnum g_qme = QMetaEnum::fromType<T>();

public:
    NODISCARD static int keyToValue(const QString &key)
    {
        return g_qme.keyToValue(mmqt::toQByteArrayLatin1(key));
    }
    NODISCARD static QString valueToKey(const int value) { return g_qme.valueToKey(value); }
    NODISCARD static int keyCount() { return g_qme.keyCount(); }
};

template<typename A, typename B>
NODISCARD static int parseTwoEnums(const QString &s)
{
    static_assert(!std::is_same_v<A, B>);
    using First = mmqt::QME<A>;
    using Second = mmqt::QME<B>;

    assert(First::keyCount() == Second::keyCount());

    // assume the enum name is actually ASCII, so Latin1 will give the correct result.
    const QByteArray arr = s.toLatin1();
    const char *const key = arr.data();

    if (const int value = First::keyToValue(key); value != -1) {
        return value;
    }
    return Second::keyToValue(key);
}

} // namespace mmqt

using westronMonthNames = mmqt::QME<MumeClock::WestronMonthNamesEnum>;
// using sindarinMonthNames = mmqt::QME<MumeClock::SindarinMonthNamesEnum>;
using westronWeekDayNames = mmqt::QME<MumeClock::WestronWeekDayNamesEnum>;
// using sindarinWeekDayNames = mmqt::QME<MumeClock::SindarinWeekDayNamesEnum>;

MumeClock::MumeClock(int64_t mumeEpoch, GameObserver &observer, QObject *const parent)
    : QObject(parent)
    , m_mumeStartEpoch(mumeEpoch)
    , m_precision(MumeClockPrecisionEnum::UNSET)
    , m_observer{observer}
{
    m_observer.sig_sentToUserGmcp.connect(m_lifetime, [this](const GmcpMessage &gmcp) {
        this->MumeClock::slot_onUserGmcp(gmcp);
    });
}

MumeClock::MumeClock(GameObserver &observer)
    : MumeClock(DEFAULT_MUME_START_EPOCH, observer, nullptr)
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
        if (!match.hasMatch()) {
            return;
        }
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
        const QString cap3 = match.captured(3);
        weekDay = getMumeWeekday(cap3);
        day = match.captured(4).toInt() - 1;
        const QString cap5 = match.captured(5);
        month = getMumeMonth(cap5);
        year = match.captured(6).toInt();
        if (m_precision <= MumeClockPrecisionEnum::DAY) {
            m_precision = MumeClockPrecisionEnum::HOUR;
        }

    } else {
        // "Highday, the 18th of Halimath, year 3030 of the Third Age."
        static const QRegularExpression rx(
            R"(^(\w+), the (\d+).{2} of (\w+), year (\d+) of the Third Age.$)");
        auto match = rx.match(mumeTime);
        if (!match.hasMatch()) {
            return;
        }

        const QString cap1 = match.captured(1);
        weekDay = getMumeWeekday(cap1);
        day = match.captured(2).toInt() - 1;
        const QString cap3 = match.captured(3);
        month = getMumeMonth(cap3);
        year = match.captured(4).toInt();
        if (m_precision <= MumeClockPrecisionEnum::UNSET) {
            m_precision = MumeClockPrecisionEnum::DAY;
        }
    }

    // Update last sync timestamp
    setLastSyncEpoch(secsSinceEpoch);

    // Calculate start of Mume epoch
    auto capturedMoment = MumeMoment(year, month, day, hour, minute);
    const int64_t mumeSecsSinceEpoch = capturedMoment.toSeconds();
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

void MumeClock::slot_onUserGmcp(const GmcpMessage &msg)
{
    if (!(msg.isEventDarkness() || msg.isEventSun()) || !msg.getJsonDocument().has_value()
        || !msg.getJsonDocument()->isObject()) {
        return;
    }

    const QJsonObject obj = msg.getJsonDocument()->object();
    if (!obj.contains("what") || !obj.value("what").isString()) {
        return;
    }

    const auto what = obj["what"].toString();
    if (what.isEmpty()) {
        return;
    }

    MumeTimeEnum time = MumeTimeEnum::UNKNOWN;
    if (msg.isEventSun()) {
        switch (what.at(0).toLatin1()) {
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
            qWarning() << "Unknown 'what' payload" << msg.toRawBytes();
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

    // Set minute to zero since all weather events happen on ticks
    auto moment = MumeMoment::sinceMumeEpoch(secsSinceEpoch - m_mumeStartEpoch);
    moment.minute = 0;

    // Predict current hour given the month
    const auto dawnDusk = getDawnDusk(moment.month);
    const int dawn = dawnDusk.dawnHour;
    const int dusk = dawnDusk.duskHour;

    const char *reason;
    switch (time) {
    case MumeTimeEnum::DAWN:
        moment.hour = dawn;
        reason = "sunrise";
        break;
    case MumeTimeEnum::DAY:
        moment.hour = dawn + 1;
        reason = "day";
        break;
    case MumeTimeEnum::DUSK:
        moment.hour = dusk;
        reason = "sunset";
        break;
    case MumeTimeEnum::NIGHT:
        moment.hour = dusk + 1;
        reason = "night";
        break;
    case MumeTimeEnum::UNKNOWN:
        // non-descriptive catch all reason
        reason = "weather";
        break;
    }

    // Update epoch
    m_mumeStartEpoch = secsSinceEpoch - moment.toSeconds();

    if (time == MumeTimeEnum::UNKNOWN && moment.minute != 0) {
        m_precision = MumeClockPrecisionEnum::DAY;
        log(QString("Unsychronized tick detected using %1 (off by %2 seconds)")
                .arg(reason)
                .arg(moment.minute));
        return;
    }

    log(QString("Synchronized tick using %1").arg(reason));
    if (time != MumeTimeEnum::UNKNOWN || m_precision >= MumeClockPrecisionEnum::HOUR) {
        m_precision = MumeClockPrecisionEnum::MINUTE;
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
    if (!match.hasMatch()) {
        return;
    }

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

void MumeClock::parseMSSP(const int year, const int month, const int day, const int hour)
{
    // We should not parse the fuzzy MSSP time if we already have a greater precision.
    if (getPrecision() > MumeClockPrecisionEnum::DAY) {
        return;
    }

    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    auto moment = getMumeMoment();
    moment.year = year;
    moment.month = month;
    moment.day = day;
    moment.hour = hour;
    // Don't override minute, since we don't get the from the MSSP time.

    const int64_t newStartEpoch = secsSinceEpoch - moment.toSeconds();
    m_mumeStartEpoch = newStartEpoch;

    // Update last sync timestamp
    setLastSyncEpoch(secsSinceEpoch);

    setPrecision(MumeClockPrecisionEnum::HOUR);
    log("Synchronized clock using MSSP");
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

    QString weekDay = westronWeekDayNames::valueToKey(moment.weekDay());
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
    const QString monthName = westronMonthNames::valueToKey(moment.month);
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
    return DawnDusk{g_dawnHour.at(m), g_duskHour.at(m)};
}

int MumeClock::getMumeMonth(const QString &monthName)
{
    return mmqt::parseTwoEnums<WestronMonthNamesEnum, SindarinMonthNamesEnum>(monthName);
}

int MumeClock::getMumeWeekday(const QString &weekdayName)
{
    return mmqt::parseTwoEnums<WestronWeekDayNamesEnum, SindarinWeekDayNamesEnum>(weekdayName);
}
