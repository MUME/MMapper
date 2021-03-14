// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "mmapper2pathmachine.h"

#include <cassert>
#include <QElapsedTimer>
#include <QString>

#include "../configuration/configuration.h"
#include "../expandoracommon/parseevent.h"
#include "pathmachine.h"
#include "pathparameters.h"

NODISCARD static const char *stateName(const PathStateEnum state)
{
#define CASE(x) \
    do { \
    case PathStateEnum::x: \
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
    const auto &settings = getConfig().pathMachine;

    params.acceptBestRelative = settings.acceptBestRelative;
    params.acceptBestAbsolute = settings.acceptBestAbsolute;
    params.newRoomPenalty = settings.newRoomPenalty;
    params.correctPositionBonus = settings.correctPositionBonus;
    params.maxPaths = settings.maxPaths;
    params.matchingTolerance = std::max(0, settings.matchingTolerance);
    params.multipleConnectionsPenalty = settings.multipleConnectionsPenalty;

    time.restart();
    emit log(me, QString("received event, state: %1").arg(stateName(state)));
    PathMachine::event(sigParseEvent);
    emit log(me,
             QString("done processing event, state: %1, elapsed: %2 ms")
                 .arg(stateName(state))
                 .arg(time.elapsed()));
}

Mmapper2PathMachine::Mmapper2PathMachine(MapData *const mapData, QObject *const parent)
    : PathMachine(mapData, parent)
{}
