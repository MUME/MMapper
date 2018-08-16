/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include "mmapper2pathmachine.h"

#include <cassert>
#include <QString>
#include <QTime>

#include "../configuration/configuration.h"
#include "../mapdata/roomfactory.h"
#include "pathmachine.h"
#include "pathparameters.h"

class SigParseEvent;

static const char *stateName(const PathState state)
{
#define CASE(x) \
    do { \
    case PathState::x: \
        return #x; \
    } while (0)
    switch (state) {
        CASE(APPROVED);
        CASE(EXPERIMENTING);
        CASE(SYNCING);
    }
#undef CASE
    assert(false);
    return "UNKNOWN";
}

void Mmapper2PathMachine::event(const SigParseEvent &sigParseEvent)
{
    static constexpr const char *const me = "PathMachine";

    /*
     * REVISIT: replace PathParameters with Configuration::PathMachineSettings
     * and then just do: params = config.pathMachine; ? 
     */
    const auto &settings = config.pathMachine;

    params.acceptBestRelative = settings.acceptBestRelative;
    params.acceptBestAbsolute = settings.acceptBestAbsolute;
    params.newRoomPenalty = settings.newRoomPenalty;
    params.correctPositionBonus = settings.correctPositionBonus;
    params.maxPaths = settings.maxPaths;
    params.matchingTolerance = settings.matchingTolerance;
    params.multipleConnectionsPenalty = settings.multipleConnectionsPenalty;

    time.restart();
    emit log(me, QString("received event, state: %1").arg(stateName(state)));
    PathMachine::event(sigParseEvent);
    emit log(me,
             QString("done processing event, state: %1, elapsed: %2 ms")
                 .arg(stateName(state))
                 .arg(time.elapsed()));
}

Mmapper2PathMachine::Mmapper2PathMachine()
    : PathMachine(new RoomFactory)
    , config(Config())
{}
