#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/RawRoom.h"
#include "abstractmapstorage.h"

#include <optional>

#include <QString>
#include <QtCore>

class MapData;
class QObject;

/*! \brief Pandora Mapper XML save loader
 *
 * This loads XML files given the schema provided in the default Pandora Mapper file:
 * https://raw.githubusercontent.com/MUME/PandoraMapper/master/deploy/mume.xml
 */
class NODISCARD_QOBJECT PandoraMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit PandoraMapStorage(const AbstractMapStorage::Data &data, QObject *parent);
    ~PandoraMapStorage() final;

public:
    PandoraMapStorage() = delete;

private:
    NODISCARD bool virt_canLoad() const final { return true; }
    NODISCARD std::optional<RawMapLoadData> virt_loadData() final;

private:
    NODISCARD bool virt_canSave() const final { return false; }
    NODISCARD bool virt_saveData(const MapLoadData & /*mapData*/) final { return false; }

private:
    struct LoadRoomHelper;
    NODISCARD ExternalRawRoom loadRoom(QXmlStreamReader &, LoadRoomHelper &);
    void loadExits(ExternalRawRoom &, QXmlStreamReader &, LoadRoomHelper &);
    void log(const QString &msg);
};
