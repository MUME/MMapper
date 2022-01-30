#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <string_view>
#include <unordered_map>
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
    RoomId loadRoomId(QXmlStreamReader &stream, const QStringView idstr);
    Coordinate loadCoordinate(QXmlStreamReader &stream);
    void loadExit(QXmlStreamReader &stream, ExitsList &exitList);
    void loadMarker(QXmlStreamReader &stream);
    void loadNotifyProgress(QXmlStreamReader &stream);

    void connectRoomsExitFrom(QXmlStreamReader &stream);
    void connectRoomExitFrom(QXmlStreamReader &stream, const Room &fromRoom, const ExitDirEnum dir);
    void moveRoomsToMapData();

    enum class RoomElementEnum : uint32_t {
        NONE /*  */ = 0,
        ALIGN /*   */ = 1 << 0,
        CONTENTS /**/ = 1 << 1,
        POSITION /**/ = 1 << 2,
        DESCRIPTION = 1 << 3,
        LIGHT /*   */ = 1 << 4,
        NOTE /*    */ = 1 << 5,
        PORTABLE /**/ = 1 << 6,
        RIDABLE /* */ = 1 << 7,
        SUNDEATH /**/ = 1 << 8,
        TERRAIN /* */ = 1 << 9,
    };

    template<typename ENUM>
    ENUM loadEnum(QXmlStreamReader &stream);
    QString loadString(QXmlStreamReader &stream);
    QStringView loadStringView(QXmlStreamReader &stream);

    static QString roomIdToString(const RoomId id);

    static void skipXmlElement(QXmlStreamReader &stream);

    static void throwError(QXmlStreamReader &stream, const QString &msg);

    // clang-format off
    template<typename... Args>
    static void throwErrorFmt(QXmlStreamReader &stream, const QString &format, Args &&... args)
    // clang-format on
    {
        throwError(stream, format.arg(std::forward<Args>(args)...));
    }

    static void throwIfDuplicate(QXmlStreamReader &stream,
                                 RoomElementEnum &set,
                                 RoomElementEnum curr);

    std::unordered_map<RoomId, SharedRoom> m_loadedRooms;
    uint64_t m_loadProgressDivisor;
    uint32_t m_loadProgress;
    static constexpr const uint32_t LOAD_PROGRESS_MAX = 100;

    // ---------------- save map -------------------
    void saveWorld(QXmlStreamWriter &stream, bool baseMapOnly);
    void saveRooms(QXmlStreamWriter &stream, bool baseMapOnly, const ConstRoomList &roomList);
    static void saveRoom(QXmlStreamWriter &stream, const Room &room);
    static void saveRoomLoadFlags(QXmlStreamWriter &stream, const RoomLoadFlags fl);
    static void saveRoomMobFlags(QXmlStreamWriter &stream, const RoomMobFlags fl);

    static void saveCoordinate(QXmlStreamWriter &stream, const QString &name, const Coordinate &pos);
    static void saveExit(QXmlStreamWriter &stream, const Exit &e, const ExitDirEnum dir);
    static void saveExitTo(QXmlStreamWriter &stream, const Exit &e);
    static void saveExitFlags(QXmlStreamWriter &stream, const ExitFlags fl);
    static void saveDoorFlags(QXmlStreamWriter &stream, const DoorFlags fl);

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
