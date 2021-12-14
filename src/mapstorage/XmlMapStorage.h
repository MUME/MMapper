#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QtCore>

#include "../global/macros.h"
#include "../mapdata/mapdata.h"
#include "abstractmapstorage.h"

class QObject;
class QXmlStreamWriter;

/*! \brief MM2 export in XML format
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
    void saveRooms(QXmlStreamWriter &stream, bool baseMapOnly, const ConstRoomList &roomList);
    static void saveRoom(QXmlStreamWriter &stream, const Room &room);
    static void saveRoomLoadFlags(QXmlStreamWriter &stream, RoomLoadFlags fl);
    static void saveRoomMobFlags(QXmlStreamWriter &stream, RoomMobFlags fl);

    static void saveCoordinate(QXmlStreamWriter &stream, const QString &name, const Coordinate &pos);
    static void saveExit(QXmlStreamWriter &stream, const Exit &e, ExitDirEnum dir);
    static void saveExitTo(QXmlStreamWriter &stream, const Exit &e);
    static void saveExitFlags(QXmlStreamWriter &stream, ExitFlags fl);
    static void saveDoorFlags(QXmlStreamWriter &stream, DoorFlags fl);

    void saveMarkers(QXmlStreamWriter &stream, const MarkerList &markerList);
    static void saveMarker(QXmlStreamWriter &stream, const InfoMark &marker);

    static void saveXmlElement(QXmlStreamWriter &stream, const QString &name, const QString &value);
    static void saveXmlAttribute(QXmlStreamWriter &stream,
                                 const QString &name,
                                 const QString &value);

    NODISCARD static const char *alignName(const RoomAlignEnum e);
    NODISCARD static const char *doorFlagName(const DoorFlagEnum e);
    NODISCARD static const char *exitFlagName(const ExitFlagEnum e);
    NODISCARD static const char *lightName(const RoomLightEnum e);
    NODISCARD static const char *loadFlagName(const RoomLoadFlagEnum e);
    NODISCARD static const char *markClassName(const InfoMarkClassEnum e);
    NODISCARD static const char *markTypeName(const InfoMarkTypeEnum e);
    NODISCARD static const char *mobFlagName(const RoomMobFlagEnum e);
    NODISCARD static const char *portableName(const RoomPortableEnum e);
    NODISCARD static const char *ridableName(const RoomRidableEnum e);
    NODISCARD static const char *sundeathName(const RoomSundeathEnum e);
    NODISCARD static const char *terrainName(const RoomTerrainEnum e);

    void log(const QString &msg);
};
