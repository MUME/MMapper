// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapstorage.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <QMessageLogContext>
#include <QObject>
#include <QtCore>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/Flags.h"
#include "../global/io.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../parser/patterns.h"
#include "StorageUtils.h"
#include "abstractmapstorage.h"
#include "basemapsavefilter.h"
#include "progresscounter.h"
#include "roomsaver.h"

static constexpr const int MMAPPER_2_0_0_SCHEMA = 17; // Initial schema
static constexpr const int MMAPPER_2_0_2_SCHEMA = 24; // Ridable flag
static constexpr const int MMAPPER_2_0_4_SCHEMA = 25; // zlib compression
static constexpr const int MMAPPER_2_3_7_SCHEMA = 32; // 16bit DoorFlags, NoMatch
static constexpr const int MMAPPER_2_4_0_SCHEMA = 33; // 16bit ExitsFlags, 32bit MobFlags/LoadFlags
static constexpr const int MMAPPER_2_4_3_SCHEMA = 34; // qCompress, SunDeath flag
static constexpr const int MMAPPER_2_5_1_SCHEMA = 35; // discard all previous NoMatch flags
static constexpr const int MMAPPER_19_10_0_SCHEMA = 36; // switches to new coordinate system
static constexpr const int CURRENT_SCHEMA = MMAPPER_19_10_0_SCHEMA;

static_assert(021 == 17, "MMapper 2.0.0 Schema");
static_assert(030 == 24, "MMapper 2.0.2 Schema");
static_assert(031 == 25, "MMapper 2.0.4 Schema");
static_assert(040 == 32, "MMapper 2.3.7 Schema");
static_assert(041 == 33, "MMapper 2.4.0 Schema");
static_assert(042 == 34, "MMapper 2.4.3 Schema");

NODISCARD static const Coordinate convertESUtoENU(Coordinate c)
{
    c.y *= -1;
    return c;
}

NODISCARD static Coordinate transformRoomOnLoad(const uint32_t version, Coordinate c)
{
    if (version <= MMAPPER_2_5_1_SCHEMA) {
        return convertESUtoENU(c);
    }
    return c;
}

static void transformInfomarkOnLoad(const uint32_t version, InfoMark &mark)
{
    if (version > MMAPPER_2_5_1_SCHEMA)
        return;

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

static void writeCoordinate(QDataStream &stream, const Coordinate &c)
{
    stream << static_cast<qint32>(c.x);
    stream << static_cast<qint32>(c.y);
    stream << static_cast<qint32>(c.z);
}

MapStorage::MapStorage(MapData &mapdata, const QString &filename, QFile *file, QObject *parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
{}

MapStorage::MapStorage(MapData &mapdata, const QString &filename, QObject *parent)
    : AbstractMapStorage(mapdata, filename, parent)
{}

void MapStorage::newData()
{
    m_mapData.unsetDataChanged();

    m_mapData.setFileName(m_fileName, false);

    Coordinate pos;
    m_mapData.setPosition(pos);

    // clear previous map
    m_mapData.clear();
    emit sig_onNewData();
}

class NODISCARD LoadRoomHelper final
{
private:
    QDataStream &stream;

public:
    explicit LoadRoomHelper(QDataStream &stream)
        : stream{stream}
    {
        check_status();
    }

public:
    void check_status()
    {
        switch (stream.status()) {
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
        stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_u16()
    {
        uint16_t result;
        stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_u32()
    {
        uint32_t result;
        stream >> result;
        check_status();
        return result;
    }
    NODISCARD auto read_i32()
    {
        int32_t result;
        stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_string()
    {
        QString result;
        stream >> result;
        check_status();
        return result;
    }

    NODISCARD auto read_datetime()
    {
        QDateTime result;
        stream >> result;
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

/* FIXME: all of these static casts need to do stronger type checking */
template<typename E>
E serialize(const uint8_t value)
{
    /* Untrusted user data; this needs bounds checking! */
    return static_cast<E>(value);
}
template<typename E>
E serialize(const uint16_t value)
{
    /* Untrusted user data; this needs bounds checking! */
    return static_cast<E>(value);
}
template<typename E>
E serialize(const uint32_t value)
{
    /* Untrusted user data; this needs bounds checking! */
    return static_cast<E>(value);
}

NODISCARD static RoomTerrainEnum serialize(const uint32_t value)
{
    if (value >= NUM_ROOM_TERRAIN_TYPES) {
        static std::once_flag flag;
        std::call_once(flag, []() { qWarning() << "Detected out of bounds terrain type!"; });
        return RoomTerrainEnum::UNDEFINED;
    }
    return static_cast<RoomTerrainEnum>(value);
}

SharedRoom MapStorage::loadRoom(QDataStream &stream, const uint32_t version)
{
    // TODO: change schema to just store size and latin1 bytes for strings.
    LoadRoomHelper helper{stream};
    const SharedRoom room = Room::createPermanentRoom(m_mapData);
    room->setName(RoomName{helper.read_string()});
    room->setDescription(RoomDesc{helper.read_string()});
    room->setContents(RoomContents{helper.read_string()});
    room->setId(RoomId{helper.read_u32() + baseId});
    room->setNote(RoomNote{helper.read_string()});
    room->setTerrainType(serialize(helper.read_u8()));
    room->setLightType(serialize<RoomLightEnum>(helper.read_u8()));
    room->setAlignType(serialize<RoomAlignEnum>(helper.read_u8()));
    room->setPortableType(serialize<RoomPortableEnum>(helper.read_u8()));
    room->setRidableType(serialize<RoomRidableEnum>(
        (version >= MMAPPER_2_0_2_SCHEMA) ? helper.read_u8() : uint8_t{0}));
    room->setSundeathType(serialize<RoomSundeathEnum>(
        (version >= MMAPPER_2_4_0_SCHEMA) ? helper.read_u8() : uint8_t{0}));
    room->setMobFlags(serialize<RoomMobFlags>(
        (version >= MMAPPER_2_4_0_SCHEMA) ? helper.read_u32() : helper.read_u16()));
    room->setLoadFlags(serialize<RoomLoadFlags>(
        (version >= MMAPPER_2_4_0_SCHEMA) ? helper.read_u32() : helper.read_u16()));
    if (helper.read_u8() /*roomUpdated*/ != 0u) {
        room->setUpToDate();
    }

    room->setPosition(transformRoomOnLoad(version, helper.readCoord3d() + basePosition));
    loadExits(*room, stream, version);
    return room;
}

void MapStorage::loadExits(Room &room, QDataStream &stream, const uint32_t version)
{
    LoadRoomHelper helper{stream};

    ExitsList eList;
    for (const ExitDirEnum i : ALL_EXITS7) {
        Exit &e = eList[i];

        // Read the exit flags
        if (version >= MMAPPER_2_4_0_SCHEMA) {
            e.setExitFlags(serialize<ExitFlags>(helper.read_u16()));
        } else {
            auto flags = serialize<ExitFlags>(helper.read_u8());
            if (flags.isDoor()) {
                flags |= ExitFlagEnum::EXIT;
            }
            e.setExitFlags(flags);
        }

        // Exits saved after MMAPPER_2_0_4_SCHEMA were offset by 1 bit causing
        // corruption and excessive NO_MATCH exits. We need to clean them and version the schema.
        if (version >= MMAPPER_2_0_4_SCHEMA && version < MMAPPER_2_5_1_SCHEMA) {
            e.setExitFlags(e.getExitFlags() & ~ExitFlags{ExitFlagEnum::NO_MATCH});
        }

        e.setDoorFlags(serialize<DoorFlags>((version >= MMAPPER_2_3_7_SCHEMA)
                                                ? helper.read_u16()
                                                : static_cast<uint16_t>(helper.read_u8())));

        e.setDoorName(static_cast<DoorName>(helper.read_string()));

        for (uint32_t connection = helper.read_u32(); connection != uint32_t(-1);
             connection = helper.read_u32()) {
            e.addIn(RoomId{connection + baseId});
        }
        for (uint32_t connection = helper.read_u32(); connection != uint32_t(-1);
             connection = helper.read_u32()) {
            e.addOut(RoomId{connection + baseId});
        }
    }

    room.setExitsList(eList);
}

bool MapStorage::loadData()
{
    // clear previous map
    m_mapData.clear();
    try {
        return mergeData();
    } catch (const std::exception &ex) {
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        log(msg);
        qWarning().noquote() << msg;
        m_mapData.clear();
        return false;
    }
}

bool MapStorage::mergeData()
{
    const auto critical = [this](const QString &msg) -> void {
        QMessageBox::critical(checked_dynamic_downcast<QWidget *>(parent()),
                              tr("MapStorage Error"),
                              msg);
    };

    {
        MapFrontendBlocker blocker(m_mapData);

        /* NOTE: This relies on the maxID being ~0u, so adding 1 brings us back to 0u. */
        baseId = m_mapData.getMaxId().asUint32() + 1u;
        basePosition = m_mapData.getMax();

        // REVISIT: What purpose does this serve?
        //
        // (Also, note that `(x + y + z) != 0` is *NOT* the same as `x != 0 || y != 0 || z != 0`,
        // so things like: [1, 2, -3] would not be affected by this.)
        if (basePosition.x + basePosition.y + basePosition.z != 0) {
            basePosition.y = 0;
            basePosition.x = 0;
            basePosition.z = -1;
        }

        log("Loading data ...");

        auto &progressCounter = getProgressCounter();
        progressCounter.reset();

        QDataStream stream(m_file);
        auto helper = LoadRoomHelper{stream};
        m_mapData.setDataChanged();

        // Read the version and magic
        static constexpr const auto MMAPPER_MAGIC = static_cast<int32_t>(0xFFB2AF01u);
        if (helper.read_i32() != MMAPPER_MAGIC) {
            return false;
        }

        const auto version = helper.read_u32();
        const bool supported = [version]() -> bool {
            switch (version) {
            case MMAPPER_2_0_0_SCHEMA:
            case MMAPPER_2_0_2_SCHEMA:
            case MMAPPER_2_0_4_SCHEMA:
            case MMAPPER_2_3_7_SCHEMA:
            case MMAPPER_2_4_0_SCHEMA:
            case MMAPPER_2_4_3_SCHEMA:
            case MMAPPER_2_5_1_SCHEMA:
            case MMAPPER_19_10_0_SCHEMA:
                return true;
            default:
                break;
            }
            return false;
        }();

        if (!supported) {
            const bool isNewer = version >= CURRENT_SCHEMA;
            critical(QString("This map has schema version %1 which is too %2.\n"
                             "\n"
                             "Please %3 MMapper.")
                         .arg(version)
                         .arg(isNewer ? "new" : "old")
                         .arg(isNewer ? "upgrade to the latest" : "try an older version of"));
            return false;
        }

        // Force serialization to Qt4.8 because Qt5 has broke backwards compatability with QDateTime serialization
        // http://doc.qt.io/qt-5/sourcebreaks.html#changes-to-qdate-qtime-and-qdatetime
        // http://doc.qt.io/qt-5/qdatastream.html#versioning
        stream.setVersion(QDataStream::Qt_4_8);

        /* Caution! Stream requires buffer to have extended lifetime for new version,
         * so don't be tempted to move this inside the scope. */
        // Then shouldn't buffer be declared before stream, so it will outlive the stream?
        QBuffer buffer;
        const bool qCompressed = (version >= MMAPPER_2_4_3_SCHEMA);
        const bool zlibCompressed = (version >= MMAPPER_2_0_4_SCHEMA
                                     && version <= MMAPPER_2_4_0_SCHEMA);
        if (qCompressed || (!NO_ZLIB && zlibCompressed)) {
            QByteArray compressedData(stream.device()->readAll());
            QByteArray uncompressedData = qCompressed ? qUncompress(compressedData)
                                                      : StorageUtils::inflate(compressedData);
            buffer.setData(uncompressedData);
            buffer.open(QIODevice::ReadOnly);
            stream.setDevice(&buffer);
            log(QString("Uncompressed map using %1").arg(qCompressed ? "qUncompress" : "zlib"));

        } else if (NO_ZLIB && zlibCompressed) {
            critical("MMapper could not load this map because it is too old.\n"
                     "\n"
                     "Please recompile MMapper with USE_ZLIB.");
            return false;
        } else {
            log("Map was not compressed");
        }
        log(QString("Schema version: %1").arg(version));

        const uint32_t roomsCount = helper.read_u32();
        const uint32_t marksCount = helper.read_u32();
        progressCounter.increaseTotalStepsBy(roomsCount + marksCount);

        m_mapData.setPosition(transformRoomOnLoad(version, helper.readCoord3d() + basePosition));

        log(QString("Number of rooms: %1").arg(roomsCount));

        for (uint32_t i = 0; i < roomsCount; ++i) {
            SharedRoom room = loadRoom(stream, version);

            progressCounter.step();
            m_mapData.insertPredefinedRoom(room);
        }

        log(QString("Number of info items: %1").arg(marksCount));

        // TODO: reserve the markerList with marksCount

        // create all pointers to items
        for (uint32_t index = 0; index < marksCount; ++index) {
            auto mark = InfoMark::alloc(m_mapData);
            loadMark(deref(mark), stream, version);
            m_mapData.addMarker(std::move(mark));

            progressCounter.step();
        }

        log("Finished loading.");

        // REVISIT: Closing is probably not necessary, since you don't do it in the failure cases.
        buffer.close();
        m_file->close();

        // REVISIT: Having a save-file with 0 rooms probably shouldn't be treated as an error.
        // (Consider just removing this test)
        if (m_mapData.getRoomsCount() == 0u) {
            return false;
        }

        m_mapData.setFileName(m_fileName, !QFileInfo(m_fileName).isWritable());
        m_mapData.unsetDataChanged();
    }

    m_mapData.checkSize();
    emit sig_onDataLoaded();
    return true;
}

void MapStorage::loadMark(InfoMark &mark, QDataStream &stream, uint32_t version)
{
    auto helper = LoadRoomHelper{stream};

    if (version <= MMAPPER_2_5_1_SCHEMA) {
        MAYBE_UNUSED const auto ignored = //
            helper.read_string();         /* value ignored; called for side effect */
    }

    mark.setText(InfoMarkText(helper.read_string()));

    if (version <= MMAPPER_2_5_1_SCHEMA) {
        MAYBE_UNUSED const auto ignored = //
            helper.read_datetime();       /* value ignored; called for side effect */
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

    if (version >= MMAPPER_2_3_7_SCHEMA) {
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
        if (version < MMAPPER_19_10_0_SCHEMA) {
            mark.setRotationAngle(helper.read_i32() / INFOMARK_SCALE);
        } else {
            mark.setRotationAngle(helper.read_i32());
        }
    }

    const auto read_coord = [](auto &helper, const auto &basePos) -> Coordinate {
        return helper.readCoord3d()
               + Coordinate{basePos.x * INFOMARK_SCALE, basePos.y * INFOMARK_SCALE, basePos.z};
    };

    mark.setPosition1(read_coord(helper, basePosition));
    mark.setPosition2(read_coord(helper, basePosition));

    transformInfomarkOnLoad(version, mark);

    if (mark.getType() != InfoMarkTypeEnum::TEXT && !mark.getText().isEmpty())
        mark.setText(InfoMarkText{});
    // REVISIT: Just discard empty text markers?
    else if (mark.getType() == InfoMarkTypeEnum::TEXT && mark.getText().isEmpty())
        mark.setText(InfoMarkText{"New Marker"});
}

void MapStorage::saveRoom(const Room &room, QDataStream &stream)
{
    stream << room.getName().toQString();
    stream << room.getDescription().toQString();
    stream << room.getContents().toQString();
    stream << static_cast<quint32>(room.getId());
    stream << room.getNote().toQString();
    stream << static_cast<quint8>(room.getTerrainType());
    stream << static_cast<quint8>(room.getLightType());
    stream << static_cast<quint8>(room.getAlignType());
    stream << static_cast<quint8>(room.getPortableType());
    stream << static_cast<quint8>(room.getRidableType());
    stream << static_cast<quint8>(room.getSundeathType());
    stream << static_cast<quint32>(room.getMobFlags());
    stream << static_cast<quint32>(room.getLoadFlags());
    stream << static_cast<quint8>(room.isUpToDate());
    writeCoordinate(stream, room.getPosition());
    saveExits(room, stream);
}

void MapStorage::saveExits(const Room &room, QDataStream &stream)
{
    const ExitsList &exitList = room.getExitsList();
    for (const Exit &e : exitList) {
        // REVISIT: need to be much more careful about how we serialize flags;
        // places like this will be easy to overlook when the definition changes
        // to increase the size beyond 16 bits.
        stream << static_cast<uint16_t>(e.getExitFlags());
        stream << static_cast<uint16_t>(e.getDoorFlags());
        stream << e.getDoorName().toQString();
        for (auto idx : e.inRange()) {
            stream << idx.asUint32();
        }
        stream << INVALID_ROOMID.asUint32();
        for (auto idx : e.outRange()) {
            stream << idx.asUint32();
        }
        stream << INVALID_ROOMID.asUint32();
    }
}

bool MapStorage::saveData(bool baseMapOnly)
{
    log("Writing data to file ...");

    QDataStream fileStream(m_file);
    fileStream.setVersion(QDataStream::Qt_4_8);

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    ConstRoomList roomList;
    roomList.reserve(m_mapData.getRoomsCount());

    const MarkerList &markerList = m_mapData.getMarkersList();
    RoomSaver saver(m_mapData, roomList);
    for (uint i = 0; i < m_mapData.getRoomsCount(); ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }

    auto roomsCount = saver.getRoomsCount();
    const auto marksCount = static_cast<uint32_t>(markerList.size());

    auto &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(roomsCount + marksCount);

    BaseMapSaveFilter filter;
    if (baseMapOnly) {
        filter.setMapData(&m_mapData);
        progressCounter.increaseTotalStepsBy(filter.prepareCount());
        filter.prepare(progressCounter);
        roomsCount = filter.acceptedRoomsCount();
    }

    // Compression step
    progressCounter.increaseTotalStepsBy(1);

    // Write a header with a "magic number" and a version
    fileStream << static_cast<quint32>(0xFFB2AF01);
    fileStream << static_cast<qint32>(CURRENT_SCHEMA);

    // Serialize the data
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_4_8);

    // write counters
    stream << static_cast<quint32>(roomsCount);
    stream << static_cast<quint32>(marksCount);

    // write selected room x,y,z
    writeCoordinate(stream, m_mapData.getPosition());

    // save rooms
    auto saveOne = [this, &stream](const Room &room) { saveRoom(room, stream); };
    for (const std::shared_ptr<const Room> &pRoom : roomList) {
        filter.visitRoom(deref(pRoom), baseMapOnly, saveOne);
        progressCounter.step();
    }

    // save items
    for (const auto &mark : markerList) {
        saveMark(deref(mark), stream);
        progressCounter.step();
    }

    buffer.close();

    QByteArray uncompressedData(buffer.data());
    QByteArray compressedData = qCompress(uncompressedData);
    progressCounter.step();
    double compressionRatio = (compressedData.isEmpty())
                                  ? 1.0
                                  : (static_cast<double>(uncompressedData.size())
                                     / static_cast<double>(compressedData.size()));
    log(QString("Map compressed (compression ratio of %1:1)")
            .arg(QString::number(compressionRatio, 'f', 1)));

    fileStream.writeRawData(compressedData.data(), compressedData.size());
    log("Writing data finished.");

    m_mapData.unsetDataChanged();
    emit sig_onDataSaved();

    return true;
}

void MapStorage::saveMark(const InfoMark &mark, QDataStream &stream)
{
    // REVISIT: save type first, and then avoid saving fields that aren't necessary?
    const InfoMarkTypeEnum type = mark.getType();
    stream << QString((type == InfoMarkTypeEnum::TEXT) ? mark.getText().toQString() : "");
    stream << static_cast<quint8>(type);
    stream << static_cast<quint8>(mark.getClass());
    // REVISIT: round to 45 degrees?
    stream << static_cast<qint32>(std::lround(mark.getRotationAngle()));
    writeCoordinate(stream, mark.getPosition1());
    writeCoordinate(stream, mark.getPosition2());
}
