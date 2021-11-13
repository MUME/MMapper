#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QHash>
#include <QList>
#include <QMetaEnum>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../global/macros.h"
#include "mumemoment.h"

class QMetaEnum;

enum class NODISCARD MumeClockPrecisionEnum { UNSET = -1, DAY, HOUR, MINUTE };

class MumeClock final : public QObject
{
    Q_OBJECT

    friend class TestClock;

public:
    static constexpr const int NUM_MONTHS = 12;
    struct NODISCARD DawnDusk
    {
        int dawnHour = 6;
        int duskHour = 18;
    };
    NODISCARD static DawnDusk getDawnDusk(int month);

public:
    explicit MumeClock(int64_t mumeEpoch, QObject *parent);

    // For use only in test cases.
    explicit MumeClock();

    NODISCARD MumeMoment getMumeMoment() const;

    NODISCARD MumeMoment getMumeMoment(int64_t secsSinceUnixEpoch) const;

    // REVISIT: This can't be const because it logs.
    NODISCARD MumeClockPrecisionEnum getPrecision();

    NODISCARD QString toMumeTime(const MumeMoment &moment) const;

    NODISCARD QString toCountdown(const MumeMoment &moment) const;

    NODISCARD int64_t getMumeStartEpoch() const { return m_mumeStartEpoch; }

    NODISCARD int64_t getLastSyncEpoch() const { return m_lastSyncEpoch; }

    enum class WestronMonthNamesEnum {
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

    enum class SindarinMonthNamesEnum {
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

    enum class WestronWeekDayNamesEnum {
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

    enum class SindarinWeekDayNamesEnum {
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

public:
    static const QMetaEnum s_westronMonthNames;
    static const QMetaEnum s_sindarinMonthNames;
    static const QMetaEnum s_westronWeekDayNames;
    static const QMetaEnum s_sindarinWeekDayNames;
    static const QHash<QString, MumeTimeEnum> m_stringTimeHash;

private:
    void log(const QString &msg) { emit sig_log("MumeClock", msg); }

signals:
    void sig_log(const QString &, const QString &);

public slots:

    void parseMumeTime(const QString &mumeTime);

    void parseClockTime(const QString &clockTime);

    void parseWeather(const QString &str);

public:
    void setPrecision(const MumeClockPrecisionEnum state);

protected:
    void parseMumeTime(const QString &mumeTime, int64_t secsSinceEpoch);

    void parseClockTime(const QString &clockTime, int64_t secsSinceEpoch);

    void parseWeather(const QString &str, int64_t secsSinceEpoch);

private:
    MumeMoment &unknownTimeTick(MumeMoment &moment);

    int64_t m_lastSyncEpoch = 0;
    int64_t m_mumeStartEpoch = 0;
    MumeClockPrecisionEnum m_precision = MumeClockPrecisionEnum::UNSET;
    int m_clockTolerance = 0;
};
