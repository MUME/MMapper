#pragma once
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

#ifndef ROOMFACTORY
#define ROOMFACTORY

#include <QString>
#include <QVector>
#include <QtCore>
#include <QtGlobal>

#include "../expandoracommon/abstractroomfactory.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "ExitDirection.h"
#include "mmapper2exit.h"

class Coordinate;
class Room;

class RoomFactory final : public AbstractRoomFactory
{
public:
    explicit RoomFactory();
    virtual Room *createRoom() const override;
    virtual Room *createRoom(const ParseEvent &) const override;
    virtual ComparisonResult compare(const Room *,
                                     const ParseEvent &event,
                                     uint tolerance) const override;
    virtual ComparisonResult compareWeakProps(const Room *,
                                              const ParseEvent &event,
                                              uint tolerance) const override;
    virtual SharedParseEvent getEvent(const Room *) const override;
    virtual void update(Room &, const ParseEvent &event) const override;
    virtual void update(Room *target, const Room *source) const override;
    virtual ~RoomFactory() override = default;

public:
    static const Coordinate &exitDir(ExitDirection dir);

private:
    ComparisonResult compareStrings(const QString &room,
                                    const QString &event,
                                    uint prevTolerance,
                                    bool updated = true) const;
};

#endif
