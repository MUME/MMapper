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
    XmlMapStorage() = delete;
    explicit XmlMapStorage(MapData &, const QString &, QFile *, QObject *parent);
    ~XmlMapStorage() final;

private:
    NODISCARD bool canLoad() const override { return true; }
    NODISCARD bool canSave() const override { return true; }

    void newData() override;
    NODISCARD bool loadData() override;
    NODISCARD bool saveData(bool baseMapOnly) override;
    NODISCARD bool mergeData() override;

    // ---------------- load map -------------------
    void loadWorld(QXmlStreamReader &stream);
    void loadMap(QXmlStreamReader &stream);
    void loadRoom(QXmlStreamReader &stream);
    Coordinate loadCoordinate(QXmlStreamReader &stream);
    void loadExit(QXmlStreamReader &stream, ExitsList &exitList);
    void loadMarker(QXmlStreamReader &stream);

    template<typename ENUM>
    ENUM loadEnum(QXmlStreamReader &stream);
    QString loadString(QXmlStreamReader &stream);
    QStringRef loadStringRef(QXmlStreamReader &stream);

    static void skipXmlElement(QXmlStreamReader &stream);

    static void throwError(const QString &msg);

    // clang-format off
    template<typename... Args>
    static void throwErrorFmt(const QString &format, Args &&... args)
    {
        throwError(format.arg(std::forward<Args>(args)...));
    }
    // clang-format on

    std::unordered_set<RoomId> roomIds;   // RoomId of loaded rooms
    std::unordered_set<RoomId> toRoomIds; // RoomId of exits

    // ---------------- save map -------------------
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

    // ---------------- misc -------------------
    void log(const QString &msg);

    enum class NODISCARD Type : uint;
    class Converter;
    static const Converter conv;
};
