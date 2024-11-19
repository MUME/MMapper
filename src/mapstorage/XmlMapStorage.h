#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../mapdata/mapdata.h"
#include "abstractmapstorage.h"

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

#include <QString>
#include <QtCore>

class QObject;
class QXmlStreamWriter;

/*! \brief MM2 export in XML format
 */
class NODISCARD_QOBJECT XmlMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    XmlMapStorage() = delete;
    explicit XmlMapStorage(const AbstractMapStorage::Data &, QObject *parent);
    ~XmlMapStorage() final;

    static inline constexpr uint32_t LOAD_PROGRESS_MAX = 100;
    struct NODISCARD Loading final
    {
        RawMapLoadData result;
        std::unordered_set<ExternalRoomId> loadedExternalRoomIds;
        std::unordered_set<ServerRoomId> loadedServerIds;
        uint64_t loadProgressDivisor = 1;
        uint32_t loadProgress = 0;
    };
    std::unique_ptr<Loading> m_loading;

    struct NODISCARD Saving final
    {
        const RawMapData &map;
        Saving() = delete;
        explicit Saving(const RawMapData &m)
            : map{m}
        {}
    };
    std::unique_ptr<Saving> m_saving;

private:
    NODISCARD bool virt_canLoad() const final { return true; }
    NODISCARD bool virt_canSave() const final { return true; }
    NODISCARD std::optional<RawMapLoadData> virt_loadData() final;
    NODISCARD bool virt_saveData(const RawMapData &map) final;

    // ---------------- load map -------------------
    void loadWorld(QXmlStreamReader &stream);
    void loadMap(QXmlStreamReader &stream);
    void loadRoom(QXmlStreamReader &stream) const;
    NODISCARD static ExternalRoomId loadExternalRoomId(QXmlStreamReader &stream, QStringView idstr);
    NODISCARD static ServerRoomId loadServerRoomId(QXmlStreamReader &stream, QStringView idstr);
    NODISCARD static Coordinate loadCoordinate(QXmlStreamReader &stream);
    static void loadExit(QXmlStreamReader &stream, ExternalRawRoom::Exits &exitList);
    void loadMarker(QXmlStreamReader &stream) const;
    void loadNotifyProgress(QXmlStreamReader &stream);

    enum class NODISCARD RoomElementEnum : uint32_t {
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
    NODISCARD static ENUM loadEnum(QXmlStreamReader &stream);
    NODISCARD static QString loadString(QXmlStreamReader &stream);
    NODISCARD static QStringView loadStringView(QXmlStreamReader &stream);

    static void skipXmlElement(QXmlStreamReader &stream);

    NORETURN
    static void throwError(QXmlStreamReader &stream, const QString &msg);

    template<typename... Args>
    NORETURN static void throwErrorFmt(QXmlStreamReader &stream,
                                       const QString &format,
                                       Args &&...args)
    {
        throwError(stream, format.arg(std::forward<Args>(args)...));
    }

    static void throwIfDuplicate(QXmlStreamReader &stream,
                                 RoomElementEnum &set,
                                 RoomElementEnum curr);

    // ---------------- save map -------------------
    void saveWorld(QXmlStreamWriter &stream);
    void saveRooms(QXmlStreamWriter &stream, const RoomIdSet &roomList);
    static void saveRoom(QXmlStreamWriter &stream, const ExternalRawRoom &room);
    static void saveRoomLoadFlags(QXmlStreamWriter &stream, RoomLoadFlags fl);
    static void saveRoomMobFlags(QXmlStreamWriter &stream, RoomMobFlags fl);

    static void saveCoordinate(QXmlStreamWriter &stream, const QString &name, const Coordinate &pos);
    static void saveExit(QXmlStreamWriter &stream, const ExternalRawExit &e, ExitDirEnum dir);
    static void saveExitTo(QXmlStreamWriter &stream, const ExternalRawExit &e);
    static void saveExitFlags(QXmlStreamWriter &stream, ExitFlags fl);
    static void saveDoorFlags(QXmlStreamWriter &stream, DoorFlags fl);

    void saveMarkers(QXmlStreamWriter &stream, const RawMarkerData &markerList);
    static void saveMarker(QXmlStreamWriter &stream, const InfoMarkFields &marker);

    static void saveXmlElement(QXmlStreamWriter &stream, const QString &name, const QString &value);
    static void saveXmlAttribute(QXmlStreamWriter &stream,
                                 const QString &name,
                                 const QString &value);

    // ---------------- misc -------------------
    void log(const QString &msg);
};
