#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QString>
#include <QtCore>

#include "../expandoracommon/coordinate.h"
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
    explicit PandoraMapStorage(MapData &, const QString &, QFile *, QObject *parent);
    ~PandoraMapStorage() final;

public:
    PandoraMapStorage() = delete;

private:
    NODISCARD bool canLoad() const override { return true; }
    NODISCARD bool canSave() const override { return false; }

    void newData() override;
    NODISCARD bool loadData() override;
    NODISCARD bool saveData(bool baseMapOnly) override;
    NODISCARD bool mergeData() override;

private:
    std::shared_ptr<Room> loadRoom(QXmlStreamReader &);
    void loadExits(Room &, QXmlStreamReader &);
    void log(const QString &msg) { emit sig_log("PandoraMapStorage", msg); }

private:
    Coordinate basePosition;
};
