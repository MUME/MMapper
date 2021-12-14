#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <unordered_set>
#include <QString>
#include <QtCore>

#include "../global/macros.h"
#include "../mapdata/mapdata.h"
#include "abstractmapstorage.h"
#include "mapstorage.h" // MapFrontendBlocker

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
    NODISCARD bool canLoad() const override { return true; }
    NODISCARD bool canSave() const override { return true; }

    void newData() override;
    NODISCARD bool loadData() override;
    NODISCARD bool saveData(bool baseMapOnly) override;
    NODISCARD bool mergeData() override;

private:
    void loadWorld(QXmlStreamReader &stream);
    void loadMap(QXmlStreamReader &stream);
    void loadRoom(QXmlStreamReader &stream);
    RoomAlignEnum loadAlign(QXmlStreamReader &stream);
    Coordinate loadCoordinate(QXmlStreamReader &stream);

    static void throwError(const QString &msg);

    template<typename... Args>
    static void throwErrorFmt(const QString &format, Args &&...args)
    {
        throwError(format.arg(std::forward<Args>(args)...));
    }

    // parse string containing an integer number (signed or unsigned).
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename T>
    static T toNumber(const QStringRef &str, bool &fail)
    {
        bool ok = false;
        if (std::is_unsigned<T>::value) {
            const ulong tmp = str.toULong(&ok);
            const T ret = static_cast<T>(tmp);
            if (!ok || static_cast<ulong>(ret) != tmp) {
                fail = true;
            }
            return ret;
        } else {
            const long tmp = str.toLong(&ok);
            const T ret = static_cast<T>(tmp);
            if (!ok || static_cast<long>(ret) != tmp) {
                fail = true;
            }
            return ret;
        }
    }

    // parse string containing the name of an enum.
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename ENUM>
    static ENUM toEnum(const QStringRef &str, bool &fail)
    {
        return ENUM(stringToEnum(enumIndex(ENUM(0)), str, fail));
    }

    // convert ENUM type to index in enumNames[]
    static constexpr uint enumIndex(RoomAlignEnum) { return 0; }
    static constexpr uint enumIndex(DoorFlagEnum) { return 1; }
    static constexpr uint enumIndex(ExitFlagEnum) { return 2; }
    static constexpr uint enumIndex(RoomLightEnum) { return 3; }
    static constexpr uint enumIndex(RoomLoadFlagEnum) { return 4; }
    static constexpr uint enumIndex(InfoMarkClassEnum) { return 5; }
    static constexpr uint enumIndex(InfoMarkTypeEnum) { return 6; }
    static constexpr uint enumIndex(RoomMobFlagEnum) { return 7; }
    static constexpr uint enumIndex(RoomPortableEnum) { return 8; }
    static constexpr uint enumIndex(RoomRidableEnum) { return 9; }
    static constexpr uint enumIndex(RoomSundeathEnum) { return 10; }
    static constexpr uint enumIndex(RoomTerrainEnum) { return 11; }

    static uint stringToEnum(uint index, const QStringRef &str, bool &fail);

    static const std::vector<std::unordered_map<QStringRef, uint>> stringToEnumMap;

    std::unordered_set<RoomId> roomIds;   // RoomId of loaded rooms
    std::unordered_set<RoomId> toRoomIds; // RoomId of exits

private:
    void saveWorld(QXmlStreamWriter &stream, bool baseMapOnly);
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
