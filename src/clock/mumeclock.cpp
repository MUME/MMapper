// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumeclock.h"

#include "../configuration/configuration.h"
#include "../global/Array.h"
#include "../global/JsonObj.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/MudTelnet.h" // FIXME: move MsspTime somewhere more appropriate, or just use MumeMoment.
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

const MMapper::Array<int, 12>
    g_dawnHour{am(8), am(9), am(8), am(7), am(7), am(6), am(5), am(4), am(5), am(6), am(7), am(7)};
const MMapper::Array<int, 12>
    g_duskHour{pm(6), pm(5), pm(6), pm(7), pm(8), pm(8), pm(9), pm(10), pm(9), pm(8), pm(8), pm(7)};

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
    NODISCARD static int keyToValue(const QString &key) { return g_qme.keyToValue(key.toUtf8()); }
    NODISCARD static QString valueToKey(const int value)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        // Qt 6.10+ changed valueToKey() signature from int to quint64
        return g_qme.valueToKey(static_cast<quint64>(value));
#else
        return g_qme.valueToKey(value);
#endif
    }
    NODISCARD static int keyCount() { return g_qme.keyCount(); }
};

template<typename A, typename B>
NODISCARD static int parseTwoEnums(const QString &s)
{
    static_assert(!std::is_same_v<A, B>);
    using First = mmqt::QME<A>;
    using Second = mmqt::QME<B>;

    assert(First::keyCount() == Second::keyCount());

    const QByteArray arr = s.toUtf8(); // assume the enum name is actually ASCII
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
    , m_observer{observer}
    , m_mumeStartEpoch(mumeEpoch)
    , m_precision(MumeClockPrecisionEnum::UNSET)
{
    m_observer.sig2_sentToUserGmcp.connect(m_lifetime,
                                           [this](const GmcpMessage &gmcp) { onUserGmcp(gmcp); });
    connect(&m_timer, &QTimer::timeout, this, &MumeClock::slot_tick);
    m_timer.start(1000);
}

MumeClock::MumeClock(GameObserver &observer)
    : MumeClock(DEFAULT_MUME_START_EPOCH, observer, nullptr)
{}

MumeMoment MumeClock::getMumeMoment() const
{
    const int64_t t = QDateTime::currentSecsSinceEpoch();
    return MumeMoment::sinceMumeEpoch(t - m_mumeStartEpoch);
}

MumeMoment MumeClock::getMumeMoment(const int64_t secsSinceUnixEpoch) const
{
    if (secsSinceUnixEpoch < 0) {
        assert(secsSinceUnixEpoch == -1);
        return getMumeMoment();
    }
    return MumeMoment::sinceMumeEpoch(secsSinceUnixEpoch - m_mumeStartEpoch);
}

void MumeClock::parseMumeTime(const QString &mumeTime)
{
    const int64_t secsSinceEpoch = QDateTime::currentSecsSinceEpoch();
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

    if (mumeTime.front().isDigit()) {
        // 3 pm on Highday, the 18th of Halimath, year 3030 of the Third Age.
        static const QRegularExpression rx(
            R"(^(\d+)(?::\d{2})?\W*(am|pm) on (\w+), the (\d+).{2} of (\w+), year (\d+) of the Third Age.$)");
        auto match = rx.match(mumeTime);
        if (!match.hasMatch()) {
            return;
        }
        hour = match.captured(1).toInt();
        if (match.captured(2).front() == 'p') {
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
    setConfig().mumeClock.startEpoch = newStartEpoch;
}

void MumeClock::onUserGmcp(const GmcpMessage &msg)
{
    if (!msg.getJsonDocument().has_value() || !(msg.isEventDarkness() || msg.isEventSun())) {
        return;
    }

    auto pObj = msg.getJsonDocument()->getObject();
    if (!pObj) {
        return;
    }
    const auto &obj = *pObj;

    const auto optWhat = obj.getString("what");
    if (!optWhat) {
        return;
    }
    const auto &what = optWhat.value();

    MumeTimeEnum time = MumeTimeEnum::UNKNOWN;
    if (msg.isEventSun()) {
        switch (mmqt::toLatin1(what.front())) {
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
    const int64_t secsSinceEpoch = QDateTime::currentSecsSinceEpoch();
    parseWeather(time, secsSinceEpoch);
}

void MumeClock::parseWeather(const MumeTimeEnum time, int64_t secsSinceEpoch)
{
    // Restart the timer to sync with the game's tick
    m_timer.start(1000);

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
    } else {
        log(QString("Synchronized tick using %1").arg(reason));
        if (time != MumeTimeEnum::UNKNOWN || m_precision >= MumeClockPrecisionEnum::HOUR) {
            m_precision = MumeClockPrecisionEnum::MINUTE;
        }
    }

    updateObserver(moment);
}

void MumeClock::parseClockTime(const QString &clockTime)
{
    const int64_t secsSinceEpoch = QDateTime::currentSecsSinceEpoch();
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
    if (match.captured(3).front() == 'p') {
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

void MumeClock::parseMSSP(const MsspTime &msspTime)
{
    // We should not parse the fuzzy MSSP time if we already have a greater precision.
    if (getPrecision() > MumeClockPrecisionEnum::DAY) {
        return;
    }

    const int64_t secsSinceEpoch = QDateTime::currentSecsSinceEpoch();

    auto moment = getMumeMoment();
    moment.year = msspTime.year;
    moment.month = msspTime.month;
    moment.day = msspTime.day;
    moment.hour = msspTime.hour;
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

void MumeClock::setLastSyncEpoch(int64_t epoch)
{
    m_lastSyncEpoch = epoch;
}

MumeClockPrecisionEnum MumeClock::getPrecision()
{
    const int64_t secsSinceEpoch = QDateTime::QDateTime::currentSecsSinceEpoch();
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
        time = QString("%1%2 on %3").arg(hour).arg(period, weekDay);
        break;
    case MumeClockPrecisionEnum::MINUTE:
        time = QString("%1:%2%3 on %4")
                   .arg(hour)
                   .arg(moment.minute, 2, 10, QChar('0'))
                   .arg(period, weekDay);
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
        .arg(QString{getOrdinalSuffix(day)}, monthName)
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

void MumeClock::slot_tick()
{
    const MumeMoment moment = getMumeMoment();
    m_observer.observeTick(moment);
    updateObserver(moment);
}

void MumeClock::updateObserver(const MumeMoment &moment)
{
    const auto timeOfDay = moment.toTimeOfDay();
    if (timeOfDay != m_timeOfDay) {
        m_timeOfDay = timeOfDay;
        m_observer.observeTimeOfDay(m_timeOfDay);
    }

    const auto moonPhase = moment.moonPhase();
    if (moonPhase != m_moonPhase) {
        m_moonPhase = moonPhase;
        m_observer.observeMoonPhase(m_moonPhase);
    }

    const auto moonVisibility = moment.moonVisibility();
    if (moonVisibility != m_moonVisibility) {
        m_moonVisibility = moonVisibility;
        m_observer.observeMoonVisibility(m_moonVisibility);
    }

    const auto season = moment.toSeason();
    if (season != m_season) {
        m_season = season;
        m_observer.observeSeason(m_season);
    }
}
