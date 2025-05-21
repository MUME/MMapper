// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapstorage.h"

#include "../global/StorageUtils.h"
#include "../global/Timer.h"
#include "../global/io.h"
#include "../global/progresscounter.h"
#include "../map/enums.h"
#include "abstractmapstorage.h"

#include <climits>
#include <cstdint>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <QMessageLogContext>
#include <QObject>
#include <QtCore>
#include <QtWidgets>

namespace { // anonymous
namespace schema {

constexpr const int v2_0_0_initial = 17;          // Initial schema (Apr 2006)
constexpr const int v2_0_2_ridable = 24;          // Ridable flag (Oct 2006)
constexpr const int v2_0_4_zlib = 25;             // zlib compression (Jun 2009)
constexpr const int v2_3_7_doorFlagsNomatch = 32; // 16-bit DoorFlags, NoMatch (Dec 2016)
constexpr const int v2_4_0_largerFlags
    = 33; // 16-bit Exits, 32-bit Mob/Load Flags, SunDeath (Dec 2017)
constexpr const int v2_4_3_qCompress = 34;      // qCompress (Jan 2018)
constexpr const int v2_5_1_discardNoMatch = 35; // discard previous NoMatch flags (Aug 2018)

// starting in 2019, versions are date based: v${YY}_${MM}_${rev}.
constexpr const int v19_10_0_newCoords = 36;      // switches to new coordinate system
constexpr const int v25_02_0_noInboundLinks = 38; // stops loading and saving inbound links
constexpr const int v25_02_1_removeUpToDate = 39; // removes upToDate
constexpr const int v25_02_2_serverId = 40;       // adds server_id
constexpr const int v25_02_3_deathFlag = 41;      // replaces death terrain with room flag
constexpr const int v25_02_4_area = 42;           // adds area

constexpr const int CURRENT = v25_02_4_area;

} // namespace schema

using enums::bitmaskToFlags;
using enums::toEnum;

NORETURN
void critical(const QString &msg)
{
    throw MapStorageError(mmqt::toStdStringUtf8(msg));
}

NODISCARD Coordinate convertESUtoENU(Coordinate c)
{
    c.y *= -1;
    return c;
}

NODISCARD Coordinate transformRoomOnLoad(const uint32_t version, Coordinate c)
{
    if (version < schema::v19_10_0_newCoords) {
        return convertESUtoENU(c);
    }
    return c;
}

void transformInfomarkOnLoad(const uint32_t version, InfoMarkFields &mark)
{
    if (version >= schema::v19_10_0_newCoords) {
        return;
    }

    static_assert(INFOMARK_SCALE == 100);
    constexpr auto TWENTIETH = INFOMARK_SCALE / 20;
    constexpr auto TENTH = INFOMARK_SCALE / 10;
    constexpr auto HALF = INFOMARK_SCALE / 2;
    const Coordinate halfRoomOffset{HALF, -HALF, 0}; // note: Y inverted

    // simple offsets in the original ESU left-handed space
    mark.setPosition1(mark.getPosition1() + halfRoomOffset);
    mark.setPosition2(mark.getPosition2() + halfRoomOffset);

    switch (mark.getType()) {
    case InfoMarkTypeEnum::TEXT: {
        const Coordinate textOffset{TENTH, 3 * TENTH, 0};
        mark.setPosition1(mark.getPosition1() + textOffset);
        mark.setPosition2(mark.getPosition2() + textOffset);
        break;
    }
    case InfoMarkTypeEnum::LINE:
        break;
    case InfoMarkTypeEnum::ARROW: {
        const Coordinate offset1{0, TWENTIETH, 0};
        const Coordinate offset2{TENTH, TENTH, 0};
        mark.setPosition1(mark.getPosition1() + offset1);
        mark.setPosition2(mark.getPosition2() + offset2);
        break;
    }
    }

    mark.setRotationAngle(-mark.getRotationAngle());

    // convert ESU to ENU
    mark.setPosition1(convertESUtoENU(mark.getPosition1()));
    mark.setPosition2(convertESUtoENU(mark.getPosition2()));
}

void writeCoordinate(QDataStream &stream, const Coordinate &c)
{
    stream << static_cast<int32_t>(c.x);
    stream << static_cast<int32_t>(c.y);
    stream << static_cast<int32_t>(c.z);
}

class NODISCARD LoadRoomHelper final
{
private:
    QDataStream &m_stream;

public:
    explicit LoadRoomHelper(QDataStream &stream)
        : m_stream{stream}
    {
        check_status();
    }

public:
    void check_status()
    {
        switch (m_stream.status()) {
        case QDataStream::Ok:
            break;
        case QDataStream::ReadPastEnd:
            throw io::IOException("read past end of file");
        case QDataStream::ReadCorruptData:
            throw io::IOException("read corrupt data");
        case QDataStream::WriteFailed:
            throw io::IOException("write failed");
        default:
            throw io::IOException("stream is not ok");
        }
    }

public:
    NODISCARD auto read_u8()
    {
        uint8_t result;
        m_stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_u16()
    {
        uint16_t result;
        m_stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_u32()
    {
        uint32_t result;
        m_stream >> result;
        check_status();
        return result;
    }
    NODISCARD auto read_i32()
    {
        int32_t result;
        m_stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_string()
    {
        QString result;
        m_stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_datetime()
    {
        QDateTime result;
        m_stream >> result;
        check_status();
        return result;
    }

    NODISCARD Coordinate readCoord3d()
    {
        const auto x = read_i32();
        const auto y = read_i32();
        const auto z = read_i32();
        return Coordinate{x, y, z};
    }
};

} // namespace

MapStorage::MapStorage(const AbstractMapStorage::Data &data, QObject *parent)
    : AbstractMapStorage{data, parent}
{}

ExternalRawRoom MapStorage::loadRoom(QDataStream &stream, const uint32_t version)
{
    // TODO: change schema to just store size and latin1 bytes for strings.
    LoadRoomHelper helper{stream};
    ExternalRawRoom room;
    room.status = RoomStatusEnum::Permanent;
    if (version >= schema::v25_02_4_area) {
        room.setArea(mmqt::makeRoomArea(helper.read_string()));
    }
    room.setName(mmqt::makeRoomName(helper.read_string()));
    room.setDescription(mmqt::makeRoomDesc(helper.read_string()));
    room.setContents(mmqt::makeRoomContents(helper.read_string()));
    room.setId(ExternalRoomId{helper.read_u32()});
    if (version >= schema::v25_02_2_serverId) {
        room.setServerId(ServerRoomId{helper.read_u32()});
    }
    room.setNote(mmqt::makeRoomNote(helper.read_string()));
    bool addDeathLoadFlag = false;
    if (version >= schema::v25_02_3_deathFlag) {
        room.setTerrainType(toEnum<RoomTerrainEnum>(helper.read_u8()));
    } else {
        const auto terrainType = helper.read_u8();
        constexpr uint8_t DEATH_TERRAIN_TYPE = 15;
        if (terrainType == DEATH_TERRAIN_TYPE) {
            addDeathLoadFlag = true;
            room.setTerrainType(RoomTerrainEnum::INDOORS);
        } else {
            room.setTerrainType(toEnum<RoomTerrainEnum>(terrainType));
        }
    }
    room.setLightType(toEnum<RoomLightEnum>(helper.read_u8()));
    room.setAlignType(toEnum<RoomAlignEnum>(helper.read_u8()));
    room.setPortableType(toEnum<RoomPortableEnum>(helper.read_u8()));
    room.setRidableType(toEnum<RoomRidableEnum>(
        (version >= schema::v2_0_2_ridable) ? helper.read_u8() : uint8_t{0}));
    room.setSundeathType(toEnum<RoomSundeathEnum>(
        (version >= schema::v2_4_0_largerFlags) ? helper.read_u8() : uint8_t{0}));
    room.setMobFlags(bitmaskToFlags<RoomMobFlags>(
        (version >= schema::v2_4_0_largerFlags) ? helper.read_u32() : helper.read_u16()));
    room.setLoadFlags(bitmaskToFlags<RoomLoadFlags>(
        (version >= schema::v2_4_0_largerFlags) ? helper.read_u32() : helper.read_u16()));
    if (addDeathLoadFlag) {
        room.addLoadFlags(RoomLoadFlagEnum::DEATHTRAP);
    }
    if (version < schema::v25_02_1_removeUpToDate) {
        /*roomUpdated*/ std::ignore = helper.read_u8();
    }

    room.setPosition(transformRoomOnLoad(version, helper.readCoord3d()));
    loadExits(room, stream, version);
    return room;
}

void MapStorage::loadExits(ExternalRawRoom &room, QDataStream &stream, const uint32_t version)
{
    LoadRoomHelper helper{stream};

    for (const ExitDirEnum i : ALL_EXITS7) {
        ExternalRawExit &e = room.exits[i];

        // Read the exit flags
        if (version >= schema::v2_4_0_largerFlags) {
            e.setExitFlags(bitmaskToFlags<ExitFlags>(helper.read_u16()));
        } else {
            e.setExitFlags(bitmaskToFlags<ExitFlags>(static_cast<uint16_t>(helper.read_u8())));
        }

        // Exits saved starting with schema::v2_0_4_zlib were offset by 1 bit causing
        // corruption and excessive NO_MATCH exits;
        // this was finally fixed with schema::v2_5_1_discardNoMatch.
        if (version >= schema::v2_0_4_zlib && version < schema::v2_5_1_discardNoMatch) {
            e.fields.exitFlags &= ~ExitFlags{ExitFlagEnum::NO_MATCH};
        }

        e.setDoorFlags(bitmaskToFlags<DoorFlags>((version >= schema::v2_3_7_doorFlagsNomatch)
                                                     ? helper.read_u16()
                                                     : static_cast<uint16_t>(helper.read_u8())));

        e.setDoorName(mmqt::makeDoorName(helper.read_string()));

        if (version < schema::v25_02_0_noInboundLinks) {
            // Inbound links will be converted to the new data format.
            for (uint32_t connection = helper.read_u32(); connection != UINT_MAX;
                 connection = helper.read_u32()) {
                e.incoming.insert(ExternalRoomId{connection});
            }
        }

        for (uint32_t connection = helper.read_u32(); connection != UINT_MAX;
             connection = helper.read_u32()) {
            e.outgoing.insert(ExternalRoomId{connection});
        }
    }
}

void MapStorage::log(const QString &msg)
{
    emit sig_log("MapStorage", msg);
    qInfo() << msg;
}

std::optional<RawMapLoadData> MapStorage::virt_loadData()
{
    DECL_TIMER(t, "MapStorage::loadData2");

    const auto &fileName = getFilename();

    RawMapLoadData result;
    result.filename = fileName;
    result.readonly = !QFileInfo(fileName).isWritable();
    auto &markerData = result.markerData.emplace();
    auto &markers = markerData.markers;

    {
        log("Loading data ...");

        auto &progressCounter = getProgressCounter();
        progressCounter.reset();

        QDataStream stream{getFile()};
        auto helper = LoadRoomHelper{stream};

        // Read the version and magic
        static constexpr const auto MMAPPER_MAGIC = static_cast<int32_t>(0xFFB2AF01u);
        if (helper.read_i32() != MMAPPER_MAGIC) {
            return std::nullopt;
        }

        const auto version = helper.read_u32();

        const bool supported = [version]() -> bool {
            switch (version) {
            case schema::v2_0_0_initial:
            case schema::v2_0_2_ridable:
            case schema::v2_0_4_zlib:
            case schema::v2_3_7_doorFlagsNomatch:
            case schema::v2_4_0_largerFlags:
            case schema::v2_4_3_qCompress:
            case schema::v2_5_1_discardNoMatch:
            case schema::v19_10_0_newCoords:
            case schema::v25_02_0_noInboundLinks:
            case schema::v25_02_1_removeUpToDate:
            case schema::v25_02_2_serverId:
            case schema::v25_02_3_deathFlag:
            case schema::v25_02_4_area:
                return true;
            default:
                break;
            }
            return false;
        }();

        if (!supported) {
            const bool isNewer = version >= schema::CURRENT;
            critical(QString("This map has schema version %1 which is too %2.\n"
                             "\n"
                             "Please %3 MMapper.")
                         .arg(version)
                         .arg(isNewer ? "new" : "old",
                              isNewer ? "upgrade to the latest" : "try an older version of"));
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code-return"
#endif
            // NOLINTNEXTLINE
            return std::nullopt; // in case critical() doesn't throw.
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        }

        // Force serialization to Qt4.8 because Qt5 has broke backwards compatability with QDateTime serialization
        // http://doc.qt.io/qt-5/sourcebreaks.html#changes-to-qdate-qtime-and-qdatetime
        // http://doc.qt.io/qt-5/qdatastream.html#versioning
        stream.setVersion(QDataStream::Qt_4_8);

        /* Caution! Stream requires buffer to have extended lifetime for new version,
         * so don't be tempted to move this inside the scope. */
        // Then shouldn't buffer be declared before stream, so it will outlive the stream?
        QBuffer buffer;
        const bool qCompressed = (version >= schema::v2_4_3_qCompress);
        const bool zlibCompressed = (version >= schema::v2_0_4_zlib
                                     && version < schema::v2_4_3_qCompress);
        if (qCompressed || (!NO_ZLIB && zlibCompressed)) {
            progressCounter.setNewTask(ProgressMsg{"uncompressing"}, 1);
            QByteArray compressedData(stream.device()->readAll());
            QByteArray uncompressedData = qCompressed
                                              ? StorageUtils::mmqt::uncompress(progressCounter,
                                                                               compressedData)
                                              : StorageUtils::mmqt::zlib_inflate(progressCounter,
                                                                                 compressedData);
            buffer.setData(uncompressedData);
            buffer.open(QIODevice::ReadOnly);
            stream.setDevice(&buffer);
            progressCounter.step();
            log(QString("Uncompressed map using %1").arg(qCompressed ? "qUncompress" : "zlib"));

        } else if (NO_ZLIB
                   // NOLINTNEXTLINE
                   && zlibCompressed) {
            // NOLINTNEXTLINE
            critical("MMapper could not load this map because it is too old.\n"
                     "\n"
                     "Please recompile MMapper with USE_ZLIB.");
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code-return"
#endif
            // NOLINTNEXTLINE
            return std::nullopt; // in case critical() doesn't throw.
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        } else {
            log("Map was not compressed");
        }
        log(QString("Schema version: %1").arg(version));

        const uint32_t roomsCount = helper.read_u32();
        const uint32_t marksCount = helper.read_u32();

        result.position = transformRoomOnLoad(version, helper.readCoord3d());

        log(QString("Number of rooms: %1").arg(roomsCount));

        progressCounter.setNewTask(ProgressMsg{"reading rooms"}, roomsCount);
        std::vector<ExternalRawRoom> loading_rooms;
        loading_rooms.reserve(roomsCount);
        for (uint32_t i = 0; i < roomsCount; ++i) {
            loading_rooms.emplace_back(loadRoom(stream, version));
            progressCounter.step();
        }

        log(QString("Number of info items: %1").arg(marksCount));

        // create all pointers to items
        progressCounter.setNewTask(ProgressMsg{"reading markers"}, marksCount);
        markers.reserve(marksCount);
        for (uint32_t index = 0; index < marksCount; ++index) {
            markers.emplace_back(loadMark(stream, version));
            progressCounter.step();
        }

        log("Finished reading rooms.");
        result.rooms = std::move(loading_rooms);
    }

    return result;
}

InfoMarkFields MapStorage::loadMark(QDataStream &stream, const uint32_t version)
{
    InfoMarkFields mark;
    auto helper = LoadRoomHelper{stream};

    if (version < schema::v19_10_0_newCoords) {
        // was name
        std::ignore = helper.read_string(); /* value ignored; called for side effect */
    }

    mark.setText(mmqt::makeInfoMarkText(helper.read_string()));

    if (version < schema::v19_10_0_newCoords) {
        // was timestamp
        std::ignore = helper.read_datetime(); /* value ignored; called for side effect */
    }

    const auto type = [](const uint8_t value) {
        if (value >= NUM_INFOMARK_TYPES) {
            static std::once_flag flag;
            std::call_once(flag, []() { qWarning() << "Detected out of bounds info mark type!"; });
            return InfoMarkTypeEnum::TEXT;
        }
        return static_cast<InfoMarkTypeEnum>(value);
    }(helper.read_u8());
    mark.setType(type);

    if (version >= schema::v2_3_7_doorFlagsNomatch) {
        const auto clazz = [](const uint8_t value) {
            if (value >= NUM_INFOMARK_CLASSES) {
                static std::once_flag flag;
                std::call_once(flag,
                               []() { qWarning() << "Detected out of bounds info mark class!"; });
                return InfoMarkClassEnum::GENERIC;
            }
            return static_cast<InfoMarkClassEnum>(value);
        }(helper.read_u8());
        mark.setClass(clazz);
        if (version < schema::v19_10_0_newCoords) {
            mark.setRotationAngle(helper.read_i32() / INFOMARK_SCALE);
        } else {
            mark.setRotationAngle(helper.read_i32());
        }
    }

    mark.setPosition1(helper.readCoord3d());
    mark.setPosition2(helper.readCoord3d());

    transformInfomarkOnLoad(version, mark);

    if (mark.getType() != InfoMarkTypeEnum::TEXT && !mark.getText().isEmpty()) {
        mark.setText(InfoMarkText{});
    }
    // REVISIT: Just discard empty text markers?
    else if (mark.getType() == InfoMarkTypeEnum::TEXT && mark.getText().isEmpty()) {
        mark.setText(InfoMarkText{"New Marker"});
    }

    return mark;
}

void MapStorage::saveRoom(const ExternalRawRoom &room, QDataStream &stream)
{
    stream << room.getArea().toQString();
    stream << room.getName().toQString();
    stream << room.getDescription().toQString();
    stream << room.getContents().toQString();
    stream << static_cast<uint32_t>(room.getId());
    stream << static_cast<uint32_t>(room.getServerId());
    stream << room.getNote().toQString();
    stream << static_cast<uint8_t>(room.getTerrainType());
    stream << static_cast<uint8_t>(room.getLightType());
    stream << static_cast<uint8_t>(room.getAlignType());
    stream << static_cast<uint8_t>(room.getPortableType());
    stream << static_cast<uint8_t>(room.getRidableType());
    stream << static_cast<uint8_t>(room.getSundeathType());
    stream << static_cast<uint32_t>(room.getMobFlags());
    stream << static_cast<uint32_t>(room.getLoadFlags());
    writeCoordinate(stream, room.getPosition());
    saveExits(room, stream);
}

void MapStorage::saveExits(const ExternalRawRoom &room, QDataStream &stream)
{
    for (const auto &e : room.getExits()) {
        // REVISIT: need to be much more careful about how we serialize flags;
        // places like this will be easy to overlook when the definition changes
        // to increase the size beyond 16 bits.
        stream << static_cast<uint16_t>(e.getExitFlags());
        stream << static_cast<uint16_t>(e.getDoorFlags());
        stream << e.getDoorName().toQString();
        for (const ExternalRoomId idx : e.getOutgoingSet()) {
            stream << static_cast<uint32_t>(idx);
        }
        stream << UINT_MAX;
    }
}

bool MapStorage::virt_saveData(const RawMapData &mapData)
{
    auto &progressCounter = getProgressCounter();
    log("Writing data to file ...");

    const auto &map = mapData.mapPair.modified;
    const RawMarkerData noMarkers;
    const RawMarkerData &markerList = mapData.markerData.has_value() ? mapData.markerData.value()
                                                                     : noMarkers;

    QDataStream fileStream(getFile());
    fileStream.setVersion(QDataStream::Qt_4_8);

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    ConstRoomList roomList;
    roomList.reserve(map.getRoomsCount());

    progressCounter.setNewTask(ProgressMsg{"scanning rooms"}, map.getRooms().size());
    for (const RoomId id : map.getRooms()) {
        const auto &room = map.getRoomHandle(id);
        if (!room.isTemporary()) {
            roomList.push_back(room);
        }
    }

    auto roomsCount = static_cast<uint32_t>(roomList.size());
    const auto marksCount = static_cast<uint32_t>(markerList.size());

    // Write a header with a "magic number" and a version
    fileStream << static_cast<uint32_t>(0xFFB2AF01);
    fileStream << static_cast<int32_t>(schema::CURRENT);

    // Serialize the data
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_4_8);

    // write counters
    stream << static_cast<uint32_t>(roomsCount);
    stream << static_cast<uint32_t>(marksCount);

    // write selected room x,y,z
    writeCoordinate(stream, mapData.position);

    // save rooms
    progressCounter.setNewTask(ProgressMsg{"saving rooms"}, roomsCount);
    for (const auto &room : roomList) {
        saveRoom(room.getRawCopyExternal(), stream);
        progressCounter.step();
    }

    // save items
    progressCounter.setNewTask(ProgressMsg{"saving markers"}, marksCount);
    for (const auto &mark : markerList.markers) {
        saveMark(mark, stream);
        progressCounter.step();
    }

    buffer.close();

    {
        progressCounter.setNewTask(ProgressMsg{"compressing"}, 1);
        QByteArray uncompressedData(buffer.data());
        QByteArray compressedData = StorageUtils::mmqt::compress(progressCounter, uncompressedData);
        progressCounter.step();

        const double compressionRatio = (compressedData.isEmpty())
                                            ? 1.0
                                            : (static_cast<double>(uncompressedData.size())
                                               / static_cast<double>(compressedData.size()));
        log(QString("Map compressed (compression ratio of %1:1)")
                .arg(QString::number(compressionRatio, 'f', 1)));

        // TODO: add progress counter
        fileStream.writeRawData(compressedData.data(), compressedData.size());
    }

    log("Writing data finished.");

    return true;
}

void MapStorage::saveMark(const InfoMarkFields &mark, QDataStream &stream)
{
    // REVISIT: save type first, and then avoid saving fields that aren't necessary?
    const InfoMarkTypeEnum type = mark.getType();
    stream << QString((type == InfoMarkTypeEnum::TEXT) ? mark.getText().toQString() : "");
    stream << static_cast<uint8_t>(type);
    stream << static_cast<uint8_t>(mark.getClass());
    // REVISIT: round to 45 degrees?
    stream << static_cast<int32_t>(std::lround(mark.getRotationAngle()));
    writeCoordinate(stream, mark.getPosition1());
    writeCoordinate(stream, mark.getPosition2());
}
