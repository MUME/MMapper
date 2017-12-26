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

#ifndef MUMEMOMENT_H
#define MUMEMOMENT_H

#define MUME_START_YEAR 2850

enum MumeTime {TIME_UNKNOWN = 0, TIME_DAWN, TIME_DAY, TIME_DUSK, TIME_NIGHT};
enum MumeSeason {SEASON_UNKNOWN = 0, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN};

class QString;

class MumeMoment
{
public:
    MumeMoment(int year, int month, int day, int hour, int minute);
    MumeMoment(int secsSinceMumeStartEpoch);

    int toSeconds();

    MumeSeason toSeason();
    MumeTime toTimeOfDay();

    int m_year = 0;
    int m_month = 0;
    int m_day = 0;
    int m_hour = 0;
    int m_minute = 0;
};

#endif // MUMEMOMENT_H
