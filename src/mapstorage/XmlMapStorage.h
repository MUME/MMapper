#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QtCore>

#include "../global/macros.h"
#include "abstractmapstorage.h"

class MapData;
class QObject;
class QXmlStreamWriter;

/*! \brief MM2 export in text (actually XML) format
 */
class XmlMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit XmlMapStorage(MapData &, const QString &, QFile *, QObject *parent);
    ~XmlMapStorage() final;

public:
    XmlMapStorage() = delete;

private:
    NODISCARD bool canLoad() const override { return false; }
    NODISCARD bool canSave() const override { return true; }

    void newData() override;
    NODISCARD bool loadData() override;
    NODISCARD bool saveData(bool baseMapOnly) override;
    NODISCARD bool mergeData() override;

private:
    static void saveXmlAttribute(QXmlStreamWriter &stream,
                                 const QString &name,
                                 const QString &value);
    static void saveXmlElement(QXmlStreamWriter &stream, const QString &name, const QString &value);
    static void saveRoom(QXmlStreamWriter &stream, const Room &room);
    static void saveRoomFlags(QXmlStreamWriter &stream, RoomLoadFlags fl);
    NODISCARD static QString terrainName(const RoomTerrainEnum x);
    NODISCARD static QString lightName(const RoomLightEnum x);
    NODISCARD static QString ridableName(const RoomRidableEnum x);
    NODISCARD static QString sundeathName(const RoomSundeathEnum x);
    NODISCARD static QString alignName(const RoomAlignEnum x);

    void log(const QString &msg) { emit sig_log("XmlMapStorage", msg); }
};
