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

#define XFOREACH_WestronMonthNamesEnum(X) \
    X(Afteryule) \
    X(Solmath) \
    X(Rethe) \
    X(Astron) \
    X(Thrimidge) \
    X(Forelithe) \
    X(Afterlithe) \
    X(Wedmath) \
    X(Halimath) \
    X(Winterfilth) \
    X(Blotmath) \
    X(Foreyule)

#define XFOREACH_SindarinMonthNamesEnum(X) \
    X(Narwain) \
    X(Ninui) \
    X(Gwaeron) \
    X(Gwirith) \
    X(Lothron) \
    X(Norui) \
    X(Cerveth) \
    X(Urui) \
    X(Ivanneth) \
    X(Narbeleth) \
    X(Hithui) \
    X(Girithron)

#define XFOREACH_WestronWeekDayNamesEnum(X) \
    X(Sunday) \
    X(Monday) \
    X(Trewsday) \
    X(Hevensday) \
    X(Mersday) \
    X(Highday) \
    X(Sterday)

#define XFOREACH_SindarinWeekDayNamesEnum(X) \
    X(Oranor) \
    X(Orithil) \
    X(Orgaladhad) \
    X(Ormenel) \
    X(Orbelain) \
    X(Oraearon) \
    X(Orgilion)

class NODISCARD_QOBJECT MumeClock final : public QObject
{
    Q_OBJECT

private:
    friend class TestClock;

private:
    int64_t m_lastSyncEpoch = 0;
    int64_t m_mumeStartEpoch = 0;
    MumeClockPrecisionEnum m_precision = MumeClockPrecisionEnum::UNSET;
    GameObserver &m_observer;

public:
    static inline constexpr const int NUM_MONTHS = 12;
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

#define X_DECL(X) X,
    enum class NODISCARD_QOBJECT WestronMonthNamesEnum : int8_t {
        Invalid = -1,
        XFOREACH_WestronMonthNamesEnum(X_DECL)
    };
    Q_ENUM(WestronMonthNamesEnum)

    enum class NODISCARD_QOBJECT SindarinMonthNamesEnum : int8_t {
        Invalid = -1,
        XFOREACH_SindarinMonthNamesEnum(X_DECL)
    };
    Q_ENUM(SindarinMonthNamesEnum)

    enum class NODISCARD_QOBJECT WestronWeekDayNamesEnum : int8_t {
        Invalid = -1,
        XFOREACH_WestronWeekDayNamesEnum(X_DECL)
    };
    Q_ENUM(WestronWeekDayNamesEnum)

    enum class NODISCARD_QOBJECT SindarinWeekDayNamesEnum : int8_t {
        Invalid = -1,
        XFOREACH_SindarinWeekDayNamesEnum(X_DECL)
    };
    Q_ENUM(SindarinWeekDayNamesEnum)
#undef X_DECL

private:
    void log(const QString &msg)
    {
        emit sig_log("MumeClock", msg);
    }

public:
    void setPrecision(MumeClockPrecisionEnum state);
    void setLastSyncEpoch(int64_t epoch)
    {
        m_lastSyncEpoch = epoch;
    }

    NODISCARD static int getMumeMonth(const QString &monthName);
    NODISCARD static int getMumeWeekday(const QString &weekdayName);
    void parseMSSP(int year, int month, int day, int hour);

protected:
    void parseMumeTime(const QString &mumeTime, int64_t secsSinceEpoch);
    void parseClockTime(const QString &clockTime, int64_t secsSinceEpoch);
    void parseWeather(MumeTimeEnum time, int64_t secsSinceEpoch);

signals:
    void sig_log(const QString &, const QString &);

public slots:
    void parseMumeTime(const QString &mumeTime);
    void parseClockTime(const QString &clockTime);
    void slot_onUserGmcp(const GmcpMessage &msg);
};
