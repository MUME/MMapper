#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef MAPSTORAGE_H
#define MAPSTORAGE_H

#include <cstdint>
#include <QArgument>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../expandoracommon/coordinate.h"
#include "../global/RuleOf5.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomfactory.h"
#include "../mapfrontend/mapfrontend.h"
#include "abstractmapstorage.h"

class Connection;
class InfoMark;
class QDataStream;
class QFile;
class QObject;
class Room;

class MapStorage : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit MapStorage(MapData &, const QString &, QFile *, QObject *parent = nullptr);
    explicit MapStorage(MapData &, const QString &, QObject *parent = nullptr);
    bool mergeData() override;

private:
    virtual bool canLoad() const override { return true; }
    virtual bool canSave() const override { return true; }

    virtual void newData() override;
    virtual bool loadData() override;
    virtual bool saveData(bool baseMapOnly) override;

    RoomFactory factory{};
    Room *loadRoom(QDataStream &stream, uint32_t version);
    void loadExits(Room &room, QDataStream &stream, uint32_t version);
    void loadMark(InfoMark *mark, QDataStream &stream, uint32_t version);
    void saveMark(InfoMark *mark, QDataStream &stream);
    void saveRoom(const Room &room, QDataStream &stream);
    void saveExits(const Room &room, QDataStream &stream);

    uint32_t baseId = 0u;
    Coordinate basePosition{};
};

class MapFrontendBlocker final
{
public:
    explicit MapFrontendBlocker(MapFrontend &in_data)
        : data(in_data)
    {
        data.block();
    }
    ~MapFrontendBlocker() { data.unblock(); }

public:
    MapFrontendBlocker() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(MapFrontendBlocker);

private:
    MapFrontend &data;
};

#endif
