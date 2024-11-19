// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "mmapper2pathmachine.h"

#include "../configuration/configuration.h"
#include "../map/parseevent.h"
#include "pathmachine.h"
#include "pathparameters.h"

#include <cassert>

#include <QElapsedTimer>
#include <QString>

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

void Mmapper2PathMachine::slot_handleParseEvent(const SigParseEvent &sigParseEvent)
{
    static constexpr const char *const me = "PathMachine";

    /*
     * REVISIT: replace PathParameters with Configuration::PathMachineSettings
     * and then just do: params = config.pathMachine; ? 
     */
    const auto &settings = getConfig().pathMachine;

    auto &params = m_params;
    // Note: clamping here isn't necessary if all writes are clamped.
    params.acceptBestRelative = settings.acceptBestRelative;
    params.acceptBestAbsolute = settings.acceptBestAbsolute;
    params.newRoomPenalty = settings.newRoomPenalty;
    params.correctPositionBonus = settings.correctPositionBonus;
    params.maxPaths = utils::clampNonNegative(settings.maxPaths);
    params.matchingTolerance = utils::clampNonNegative(settings.matchingTolerance);
    params.multipleConnectionsPenalty = settings.multipleConnectionsPenalty;

    m_time.restart();
    emit sig_log(me, QString("received event, state: %1").arg(stateName(m_state)));
    PathMachine::handleParseEvent(sigParseEvent);
    emit sig_log(me,
                 QString("done processing event, state: %1, elapsed: %2 ms")
                     .arg(stateName(m_state))
                     .arg(m_time.elapsed()));
}

Mmapper2PathMachine::Mmapper2PathMachine(MapData *const mapData, QObject *const parent)
    : PathMachine(mapData, parent)
{}
