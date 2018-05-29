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

#include "abstractroomfactory.h"
#include <QVector>
#include <QRegExp>

class RoomFactory : public AbstractRoomFactory
{
public:
    RoomFactory();
    virtual Room *createRoom(const ParseEvent *ev = 0) const override;
    virtual ComparisonResult compare(const Room *, const ParseEvent *event,
                                     uint tolerance = 0) const override;
    virtual ComparisonResult compareWeakProps(const Room *, const ParseEvent &event,
                                              uint tolerance = 0) const override;
    virtual ParseEvent *getEvent(const Room *) const override;
    virtual void update(Room &, const ParseEvent &event) const override;
    virtual void update(Room *target, const Room *source) const override;
    virtual uint opposite(uint in) const override;
    virtual const Coordinate &exitDir(uint dir) const override;
    virtual uint numKnownDirs() const override
    {
        return 8;
    }
    virtual ~RoomFactory() {}
private:
    static const QRegExp whitespace;
    QVector<Coordinate> exitDirs;
    ComparisonResult compareStrings(const QString &room, const QString &event, uint prevTolerance,
                                    bool updated = true) const;
};

#endif
