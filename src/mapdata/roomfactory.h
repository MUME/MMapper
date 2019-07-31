#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QString>
#include <QVector>
#include <QtCore>
#include <QtGlobal>

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "ExitDirection.h"
#include "mmapper2exit.h"

class Coordinate;
class Room;

class RoomFactory final : public AbstractRoomFactory
{
public:
    RoomFactory();
    virtual Room *createRoom() const override;
    virtual Room *createRoom(const ParseEvent &) const override;
    virtual ComparisonResultEnum compare(const Room *,
                                         const ParseEvent &event,
                                         int tolerance) const override;
    virtual ComparisonResultEnum compareWeakProps(const Room *,
                                                  const ParseEvent &event,
                                                  int tolerance) const override;
    virtual SharedParseEvent getEvent(const Room *) const override;
    virtual void update(Room &, const ParseEvent &event) const override;
    virtual void update(Room *target, const Room *source) const override;
    virtual ~RoomFactory() override = default;

public:
    static const Coordinate &exitDir(ExitDirEnum dir);

private:
    static ComparisonResultEnum compareStrings(const std::string &room,
                                               const std::string &event,
                                               int prevTolerance,
                                               bool updated = true);
};
