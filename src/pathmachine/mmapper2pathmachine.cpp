// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "mmapper2pathmachine.h"

#include "../configuration/configuration.h"
#include "../global/SendToUser.h"
#include "../map/parseevent.h"
#include "../mapdata/mapdata.h"
#include "pathmachine.h"
#include "pathparameters.h"

#include <cassert>

#include <QElapsedTimer>
#include <QString>

NODISCARD static const char *stateName(const PathStateEnum state)
{
    if (state == PathStateEnum::APPROVED) {
        return "";
    }
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

    // Extract prompt flags and update MapData for dynamic lighting
    const ParseEvent &event = sigParseEvent.deref();
    const PromptFlagsType promptFlags = event.getPromptFlags();
    if (auto *mapData = dynamic_cast<MapData *>(&getMap())) {
        mapData->setPromptFlags(promptFlags);
    }

    try {
        PathMachine::handleParseEvent(sigParseEvent);
    } catch (const std::exception &e) {
        global::sendToUser(QString("ERROR: %1\n").arg(e.what()));
    } catch (...) {
        global::sendToUser("ERROR: unknown exception\n");
    }

    emit sig_state(stateName(getState()));
}

Mmapper2PathMachine::Mmapper2PathMachine(MapFrontend &map, QObject *const parent)
    : PathMachine(map, parent)
{}
