#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

    MumeClockPrecision getPrecision();

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

    enum class WestronWeekDayNames {
        UnknownWestronWeekDay = -1,
        Sunday,
        Monday,
        Trewsday,
        Hevensday,
        Mersday,
        Highday,
        Sterday
    };

    Q_ENUM(WestronWeekDayNames)

    enum class SindarinWeekDayNames {
        UnknownSindarinWeekDay = -1,
        Oranor,
        Orithil,
        Orgaladhad,
        Ormenel,
        Orbelain,
        Oraearon,
        Orgilion
    };

    Q_ENUM(SindarinWeekDayNames)

private:
    static const std::array<int, NUM_MONTHS> s_dawnHour;
    static const std::array<int, NUM_MONTHS> s_duskHour;

public:
    static const QMetaEnum s_westronMonthNames;
    static const QMetaEnum s_sindarinMonthNames;
    static const QMetaEnum s_westronWeekDayNames;
    static const QMetaEnum s_sindarinWeekDayNames;
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

    int64_t m_lastSyncEpoch = 0;
    int64_t m_mumeStartEpoch = 0;
    MumeClockPrecision m_precision{};
    int m_clockTolerance = 0;
};
