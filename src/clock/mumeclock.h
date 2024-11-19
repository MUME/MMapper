#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../observer/gameobserver.h"
#include "mumemoment.h"

#include <cstdint>

#include <QList>
#include <QMetaEnum>
#include <QObject>
#include <QString>
#include <QtCore>

class GmcpMessage;
class QMetaEnum;

enum class NODISCARD MumeClockPrecisionEnum : int8_t { UNSET = -1, DAY, HOUR, MINUTE };

class MumeClock final : public QObject
{
    Q_OBJECT

    friend class TestClock;

private:
    int64_t m_lastSyncEpoch = 0;
    int64_t m_mumeStartEpoch = 0;
    MumeClockPrecisionEnum m_precision = MumeClockPrecisionEnum::UNSET;
    GameObserver &m_observer;

public:
    static constexpr const int NUM_MONTHS = 12;
    struct NODISCARD DawnDusk
    {
        int dawnHour = 6;
        int duskHour = 18;
    };
    NODISCARD static DawnDusk getDawnDusk(int month);

public:
    explicit MumeClock(int64_t mumeEpoch, GameObserver &observer, QObject *parent);

    // For use only in test cases.
    explicit MumeClock(GameObserver &observer);

    NODISCARD MumeMoment getMumeMoment() const;
    NODISCARD MumeMoment getMumeMoment(int64_t secsSinceUnixEpoch) const;

    // REVISIT: This can't be const because it logs.
    NODISCARD MumeClockPrecisionEnum getPrecision();

    NODISCARD QString toMumeTime(const MumeMoment &moment) const;
    NODISCARD QString toCountdown(const MumeMoment &moment) const;

    NODISCARD int64_t getMumeStartEpoch() const { return m_mumeStartEpoch; }
    NODISCARD int64_t getLastSyncEpoch() const { return m_lastSyncEpoch; }

    enum class WestronMonthNamesEnum : int8_t {
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

    Q_ENUM(WestronMonthNamesEnum)

    enum class SindarinMonthNamesEnum : int8_t {
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

    Q_ENUM(SindarinMonthNamesEnum)

    enum class WestronWeekDayNamesEnum : int8_t {
        UnknownWestronWeekDay = -1,
        Sunday,
        Monday,
        Trewsday,
        Hevensday,
        Mersday,
        Highday,
        Sterday
    };

    Q_ENUM(WestronWeekDayNamesEnum)

    enum class SindarinWeekDayNamesEnum : int8_t {
        UnknownSindarinWeekDay = -1,
        Oranor,
        Orithil,
        Orgaladhad,
        Ormenel,
        Orbelain,
        Oraearon,
        Orgilion
    };

    Q_ENUM(SindarinWeekDayNamesEnum)

private:
    void log(const QString &msg) { emit sig_log("MumeClock", msg); }

signals:
    void sig_log(const QString &, const QString &);

public slots:
    void parseMumeTime(const QString &mumeTime);
    void parseClockTime(const QString &clockTime);
    void slot_onUserGmcp(const GmcpMessage &msg);

public:
    void setPrecision(MumeClockPrecisionEnum state);
    void setLastSyncEpoch(int64_t epoch) { m_lastSyncEpoch = epoch; }

    NODISCARD static int getMumeMonth(const QString &monthName);
    void parseMSSP(int year, int month, int day, int hour);

protected:
    void parseMumeTime(const QString &mumeTime, int64_t secsSinceEpoch);
    void parseClockTime(const QString &clockTime, int64_t secsSinceEpoch);
    void parseWeather(MumeTimeEnum time, int64_t secsSinceEpoch);
};
