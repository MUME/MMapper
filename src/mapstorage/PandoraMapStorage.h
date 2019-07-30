#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
class PandoraMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit PandoraMapStorage(MapData &, const QString &, QFile *, QObject *parent = nullptr);
    ~PandoraMapStorage() override;

public:
    PandoraMapStorage() = delete;

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
    RoomFactory factory;
    Coordinate basePosition;
};
