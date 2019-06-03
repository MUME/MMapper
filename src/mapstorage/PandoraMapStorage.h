#pragma once
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

#ifndef PANDORAMAPSTORAGE_H
#define PANDORAMAPSTORAGE_H

#include <QString>
#include <QtCore>

#include "../expandoracommon/coordinate.h"
#include "../mapdata/roomfactory.h"
#include "abstractmapstorage.h"

class MapData;
class QObject;

/*! \brief Pandora Mapper XML save loader
 *
 * This loads XML files given the schema provided in the default Pandora Mapper file:
 * https://raw.githubusercontent.com/MUME/PandoraMapper/master/deploy/mume.xml
 */
class PandoraMapStorage : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit PandoraMapStorage(MapData &, const QString &, QFile *, QObject *parent = nullptr);
    ~PandoraMapStorage() override;

public:
    explicit PandoraMapStorage() = delete;

private:
    virtual bool canLoad() const override { return true; }
    virtual bool canSave() const override { return false; }

    virtual void newData() override;
    virtual bool loadData() override;
    virtual bool saveData(bool baseMapOnly) override;
    virtual bool mergeData() override;

private:
    Room *loadRoom(QXmlStreamReader &);
    void loadExits(Room &, QXmlStreamReader &);

private:
    RoomFactory factory{};

    Coordinate basePosition{};
};

#endif // PANDORAMAPSTORAGE_H
