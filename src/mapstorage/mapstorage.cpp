/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "mapstorage.h"

#include <climits>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <QMessageLogContext>
#include <QObject>
#include <QtCore>
#include <QtWidgets>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/Flags.h"
#include "../global/bits.h"
#include "../global/io.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomfactory.h"
#include "../parser/patterns.h"
#include "abstractmapstorage.h"
#include "basemapsavefilter.h"
#include "oldconnection.h"
#include "olddoor.h"
#include "oldroom.h"
#include "progresscounter.h"
#include "roomsaver.h"

#ifndef MMAPPER_NO_QTIOCOMPRESSOR
#include "qtiocompressor.h"

static_assert(std::is_same<uint32_t, decltype(UINT_MAX)>::value, "");

static constexpr const bool USE_IO_COMPRESSOR = true;
#else
static constexpr const bool USE_IO_COMPRESSOR = false;
/* NOTE: Once we require C++17, this can be if constexpr,
* and it won't be necessary to mock QtIOCompressor. */
#include <QIODevice>
using OpenMode = QIODevice::OpenMode;
class QtIOCompressor final : public QIODevice
{
public:
    explicit QtIOCompressor(QFile *) { std::abort(); }

public:
    bool open(OpenMode /*mode*/) override
    {
        std::abort();
        return false;
    }
    void close() override { std::abort(); }

protected:
    qint64 readData(char * /*data*/, qint64 /*maxlen*/) override
    {
        std::abort();
        return 0;
    }

    qint64 writeData(const char * /*data*/, qint64 /*len*/) override
    {
        std::abort();
        return 0;
    }
};
#endif

static constexpr const int MINUMUM_STATIC_LINES = 1;

// TODO(nschimme): Strip out support for older maps predating MMapper2
static constexpr const int MMAPPER_1_0_0_SCHEMA = 7;  // MMapper 1.0 ???
static constexpr const int MMAPPER_1_1_0_SCHEMA = 16; // MMapper 1.1 ???
static constexpr const int MMAPPER_2_0_0_SCHEMA = 17; // Initial schema
static constexpr const int MMAPPER_2_0_2_SCHEMA = 24; // Ridable flag
static constexpr const int MMAPPER_2_0_4_SCHEMA = 25; // QtIOCompressor
static constexpr const int MMAPPER_2_3_7_SCHEMA = 32; // 16bit DoorFlags, NoMatch
static constexpr const int MMAPPER_2_4_0_SCHEMA = 33; // 16bit ExitsFlags, 32bit MobFlags/LoadFlags
static constexpr const int MMAPPER_2_4_3_SCHEMA = 34; // qCompress, SunDeath flag
static constexpr const int MMAPPER_2_5_1_SCHEMA = 35; // discard all previous NoMatch flags
static constexpr const int CURRENT_SCHEMA = MMAPPER_2_5_1_SCHEMA;

static_assert(007 == 7, "MMapper 1.0.0 Schema");
static_assert(020 == 16, "MMapper 1.1.0 Schema");
static_assert(021 == 17, "MMapper 2.0.0 Schema");
static_assert(030 == 24, "MMapper 2.0.2 Schema");
static_assert(031 == 25, "MMapper 2.0.4 Schema");
static_assert(040 == 32, "MMapper 2.3.7 Schema");
static_assert(041 == 33, "MMapper 2.4.0 Schema");
static_assert(042 == 34, "MMapper 2.4.3 Schema");

MapStorage::MapStorage(MapData &mapdata, const QString &filename, QFile *file, QObject *parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
{}

MapStorage::MapStorage(MapData &mapdata, const QString &filename, QObject *parent)
    : AbstractMapStorage(mapdata, filename, parent)
{}

void MapStorage::newData()
{
    m_mapData.unsetDataChanged();

    m_mapData.setFileName(m_fileName);

    Coordinate pos;
    m_mapData.setPosition(pos);

    //clear previous map
    m_mapData.clear();
    emit onNewData();
}

static void setConnection(Connection *c, Room *room, ConnectionDirection cd)
{
    c->setRoom(room, c->getRoom(Hand::LEFT) != nullptr ? Hand::RIGHT : Hand::LEFT);
    c->setDirection(cd, room);
}

class LoadRoomHelperBase
{
private:
    QDataStream &stream;

public:
    explicit LoadRoomHelperBase(QDataStream &stream)
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
    auto read_u8()
    {
        uint8_t result;
        stream >> result;
        check_status();
        return result;
    }

    auto read_u16()
    {
        uint16_t result;
        stream >> result;
        check_status();
        return result;
    }

    auto read_u32()
    {
        uint32_t result;
        stream >> result;
        return result;
    }
    auto read_i32()
    {
        int32_t result;
        stream >> result;
        check_status();
        return result;
    }

    auto read_string()
    {
        QString result;
        stream >> result;
        check_status();
        return result;
    }

    auto read_datetime()
    {
        QDateTime result;
        stream >> result;
        check_status();
        return result;
    }
};

class OldLoadRoomHelper : public LoadRoomHelperBase
{
public:
    explicit OldLoadRoomHelper(QDataStream &stream)
        : LoadRoomHelperBase{stream}
    {}

public:
    auto readRoomAlignType() { return static_cast<RoomAlignType>(read_u16() + 1u); }
    auto readRoomLightType() { return static_cast<RoomLightType>(read_u16() + 1u); }
    auto readRoomPortableType() { return static_cast<RoomPortableType>(read_u16() + 1u); }
    auto readRoomTerrainType() { return static_cast<RoomTerrainType>(read_u16() + 1u); }

    Coordinate readCoord2d()
    {
        Coordinate pos;
        pos.x = read_i32();
        pos.y = read_i32();
        return pos;
    }

    RoomMobFlags readMobFlags()
    {
        switch (static_cast<int>(read_u16())) {
        case 1:
            return RoomMobFlags{RoomMobFlag::ANY}; // PEACEFUL
        case 2:
            return RoomMobFlags{RoomMobFlag::SMOB}; // AGGRESIVE
        case 3:
            return RoomMobFlags{RoomMobFlag::QUEST};
        case 4:
            return RoomMobFlags{RoomMobFlag::SHOP};
        case 5:
            return RoomMobFlags{RoomMobFlag::RENT};
        case 6:
            return RoomMobFlags{RoomMobFlag::GUILD};
        default:
            return RoomMobFlags{0};
        }
    }

    RoomLoadFlags readLoadFlags()
    {
        switch (static_cast<int>(read_u16())) {
        case 1:
            return RoomLoadFlags{RoomLoadFlag::TREASURE};
        case 2:
            return RoomLoadFlags{RoomLoadFlag::HERB};
        case 3:
            return RoomLoadFlags{RoomLoadFlag::KEY};
        case 4:
            return RoomLoadFlags{RoomLoadFlag::WATER};
        case 5:
            return RoomLoadFlags{RoomLoadFlag::FOOD};
        case 6:
            return RoomLoadFlags{RoomLoadFlag::HORSE};
        case 7:
            return RoomLoadFlags{RoomLoadFlag::WARG};
        case 8:
            return RoomLoadFlags{RoomLoadFlag::TOWER};
        case 9:
            return RoomLoadFlags{RoomLoadFlag::ATTENTION};
        case 10:
            return RoomLoadFlags{RoomLoadFlag::BOAT};
        default:
            return RoomLoadFlags{0};
        }
    }
};

struct OldLoadRoomHelper2 : public OldLoadRoomHelper
{
private:
    ConnectionList &connectionList;

public:
    explicit OldLoadRoomHelper2(QDataStream &stream, ConnectionList &connectionList)
        : OldLoadRoomHelper{stream}
        , connectionList{connectionList}
    {}

    Connection *readConnection()
    {
        if (auto tmp = read_u32()) {
            auto c = connectionList[tmp - 1u];
            return c;
        }
        return nullptr;
    }
};

Room *MapStorage::loadOldRoom(QDataStream &stream, ConnectionList &connectionList)
{
    auto helper = OldLoadRoomHelper2{stream, connectionList};
    Room *const room = factory.createRoom();
    room->setPermanent();

    const auto ridableType = RoomRidableType::UNDEFINED;
    const auto sundeathType = RoomSundeathType::UNDEFINED;

    room->setName(helper.read_string());

    {
        bool readingStaticDescLines = true;
        QString staticRoomDesc{};
        QString dynamicRoomDesc{};
        qint32 lineCount = 0u;
        for (const auto &s : helper.read_string().split('\n')) {
            if (s.isEmpty())
                continue;

            // REVISIT: simplify this!
            // This logic is apparently an overly complicated way of saying
            // that the first line is guaranteed to go into static,
            // but all the rest go to dynamic once one matches the pattern.
            if ((lineCount++ >= MINUMUM_STATIC_LINES)
                && ((!readingStaticDescLines) || Patterns::matchDynamicDescriptionPatterns(s))) {
                readingStaticDescLines = false;
                dynamicRoomDesc += (s) + "\n";
            } else {
                staticRoomDesc += (s) + "\n";
            }
        }
        room->setStaticDescription(staticRoomDesc);
        room->setDynamicDescription(dynamicRoomDesc);
    }

    const auto terrainType = helper.readRoomTerrainType();
    const auto mobFlags = helper.readMobFlags();
    const auto loadFlags = helper.readLoadFlags();
    (void) helper.read_u16();                                //roomLocation { INDOOR, OUTSIDE }
    const auto portableType = helper.readRoomPortableType(); //roomPortable { PORT, NOPORT }
    const auto lightType = helper.readRoomLightType();       //roomLight { DARK, LIT }
    const auto alignType = helper.readRoomAlignType();       //roomAlign { GOOD, NEUTRAL, EVIL }

    {
        const auto roomFlags = helper.read_u32(); //roomFlags

        if (IS_SET(roomFlags, bit2)) {
            room->exit(ExitDirection::NORTH).orExitFlags(ExitFlag::EXIT);
        }
        if (IS_SET(roomFlags, bit3)) {
            room->exit(ExitDirection::SOUTH).orExitFlags(ExitFlag::EXIT);
        }
        if (IS_SET(roomFlags, bit4)) {
            room->exit(ExitDirection::EAST).orExitFlags(ExitFlag::EXIT);
        }
        if (IS_SET(roomFlags, bit5)) {
            room->exit(ExitDirection::WEST).orExitFlags(ExitFlag::EXIT);
        }
        if (IS_SET(roomFlags, bit6)) {
            room->exit(ExitDirection::UP).orExitFlags(ExitFlag::EXIT);
        }
        if (IS_SET(roomFlags, bit7)) {
            room->exit(ExitDirection::DOWN).orExitFlags(ExitFlag::EXIT);
        }
        if (IS_SET(roomFlags, bit8)) {
            room->exit(ExitDirection::NORTH).orExitFlags(ExitFlag::DOOR);
            room->exit(ExitDirection::NORTH).orExitFlags(ExitFlag::NO_MATCH);
        }
        if (IS_SET(roomFlags, bit9)) {
            room->exit(ExitDirection::SOUTH).orExitFlags(ExitFlag::DOOR);
            room->exit(ExitDirection::SOUTH).orExitFlags(ExitFlag::NO_MATCH);
        }
        if (IS_SET(roomFlags, bit10)) {
            room->exit(ExitDirection::EAST).orExitFlags(ExitFlag::DOOR);
            room->exit(ExitDirection::EAST).orExitFlags(ExitFlag::NO_MATCH);
        }
        if (IS_SET(roomFlags, bit11)) {
            room->exit(ExitDirection::WEST).orExitFlags(ExitFlag::DOOR);
            room->exit(ExitDirection::WEST).orExitFlags(ExitFlag::NO_MATCH);
        }
        if (IS_SET(roomFlags, bit12)) {
            room->exit(ExitDirection::UP).orExitFlags(ExitFlag::DOOR);
            room->exit(ExitDirection::UP).orExitFlags(ExitFlag::NO_MATCH);
        }
        if (IS_SET(roomFlags, bit13)) {
            room->exit(ExitDirection::DOWN).orExitFlags(ExitFlag::DOOR);
            room->exit(ExitDirection::DOWN).orExitFlags(ExitFlag::NO_MATCH);
        }
        if (IS_SET(roomFlags, bit14)) {
            room->exit(ExitDirection::NORTH).orExitFlags(ExitFlag::ROAD);
        }
        if (IS_SET(roomFlags, bit15)) {
            room->exit(ExitDirection::SOUTH).orExitFlags(ExitFlag::ROAD);
        }
        if (IS_SET(roomFlags, bit16)) {
            room->exit(ExitDirection::EAST).orExitFlags(ExitFlag::ROAD);
        }
        if (IS_SET(roomFlags, bit17)) {
            room->exit(ExitDirection::WEST).orExitFlags(ExitFlag::ROAD);
        }
        if (IS_SET(roomFlags, bit18)) {
            room->exit(ExitDirection::UP).orExitFlags(ExitFlag::ROAD);
        }
        if (IS_SET(roomFlags, bit19)) {
            room->exit(ExitDirection::DOWN).orExitFlags(ExitFlag::ROAD);
        }
    }

    (void) helper.read_u8(); //roomUpdated
    (void) helper.read_u8(); //roomCheckExits

    {
        const Coordinate pos = helper.readCoord2d();
        room->setPosition(pos + basePosition);
    }

    if (auto c = helper.readConnection()) {
        setConnection(c, room, ConnectionDirection::UP);
    }
    if (auto c = helper.readConnection()) {
        setConnection(c, room, ConnectionDirection::DOWN);
    }
    if (auto c = helper.readConnection()) {
        setConnection(c, room, ConnectionDirection::EAST);
    }
    if (auto c = helper.readConnection()) {
        setConnection(c, room, ConnectionDirection::WEST);
    }
    if (auto c = helper.readConnection()) {
        setConnection(c, room, ConnectionDirection::NORTH);
    }
    if (auto c = helper.readConnection()) {
        setConnection(c, room, ConnectionDirection::SOUTH);
    }

    // store imported values
    room->setTerrainType(terrainType);
    room->setLightType(lightType);
    room->setAlignType(alignType);
    room->setPortableType(portableType);
    room->setRidableType(ridableType);
    room->setSundeathType(sundeathType);
    room->setMobFlags(mobFlags);
    room->setLoadFlags(loadFlags);

    return room;
}

class NewLoadRoomHelper final : public LoadRoomHelperBase
{
public:
    explicit NewLoadRoomHelper(QDataStream &stream)
        : LoadRoomHelperBase{stream}
    {}

public:
    Coordinate readCoord3d()
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

Room *MapStorage::loadRoom(QDataStream &stream, qint32 version)
{
    auto helper = NewLoadRoomHelper{stream};
    Room *room = factory.createRoom();
    room->setPermanent();
    room->setName(helper.read_string());
    room->setStaticDescription(helper.read_string());
    room->setDynamicDescription(helper.read_string());
    room->setId(RoomId{helper.read_u32() + baseId});
    room->setNote(helper.read_string());
    room->setTerrainType(serialize<RoomTerrainType>(helper.read_u8()));
    room->setLightType(serialize<RoomLightType>(helper.read_u8()));
    room->setAlignType(serialize<RoomAlignType>(helper.read_u8()));
    room->setPortableType(serialize<RoomPortableType>(helper.read_u8()));
    room->setRidableType(serialize<RoomRidableType>(
        (version >= MMAPPER_2_0_2_SCHEMA) ? helper.read_u8() : uint8_t{0}));
    room->setSundeathType(serialize<RoomSundeathType>(
        (version >= MMAPPER_2_4_0_SCHEMA) ? helper.read_u8() : uint8_t{0}));
    room->setMobFlags(serialize<RoomMobFlags>(
        (version >= MMAPPER_2_4_0_SCHEMA) ? helper.read_u32() : helper.read_u16()));
    room->setLoadFlags(serialize<RoomLoadFlags>(
        (version >= MMAPPER_2_4_0_SCHEMA) ? helper.read_u32() : helper.read_u16()));
    if (helper.read_u8() /*roomUpdated*/ != 0u) {
        room->setUpToDate();
    }
    room->setPosition(helper.readCoord3d() + basePosition);
    loadExits(*room, stream, version);
    return room;
}

void MapStorage::loadExits(Room &room, QDataStream &stream, const qint32 version)
{
    auto helper = NewLoadRoomHelper{stream};

    ExitsList &eList = room.getExitsList();
    for (const auto i : ALL_EXITS7) {
        Exit &e = eList[i];

        // Read the exit flags
        if (version >= MMAPPER_2_4_0_SCHEMA) {
            e.setExitFlags(serialize<ExitFlags>(helper.read_u16()));
        } else {
            auto flags = serialize<ExitFlags>(helper.read_u8());
            if (flags.isDoor()) {
                flags |= ExitFlag::EXIT;
            }
            e.setExitFlags(flags);
        }

        // Exits saved after MMAPPER_2_0_4_SCHEMA were offset by 1 bit causing
        // corruption and excessive NO_MATCH exits. We need to clean them and version the schema.
        if (version >= MMAPPER_2_0_4_SCHEMA && version < MMAPPER_2_5_1_SCHEMA) {
            e.setExitFlags(e.getExitFlags() & ~ExitFlags{ExitFlag::NO_MATCH});
        }

        e.setDoorFlags(serialize<DoorFlags>((version >= MMAPPER_2_3_7_SCHEMA)
                                                ? helper.read_u16()
                                                : static_cast<uint16_t>(helper.read_u8())));

        e.setDoorName(static_cast<DoorName>(helper.read_string()));

        for (uint32_t connection = helper.read_u32(); connection != UINT_MAX;
             connection = helper.read_u32()) {
            e.addIn(RoomId{connection + baseId});
        }
        for (uint32_t connection = helper.read_u32(); connection != UINT_MAX;
             connection = helper.read_u32()) {
            e.addOut(RoomId{connection + baseId});
        }
    }
}

bool MapStorage::loadData()
{
    //clear previous map
    m_mapData.clear();
    try {
        return mergeData();
    } catch (const std::exception &ex) {
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        emit log("MapStorage", msg);
        qWarning().noquote() << msg;
        m_mapData.clear();
        return false;
    }
}

bool MapStorage::mergeData()
{
    {
        MapFrontendBlocker blocker(m_mapData);

        /* NOTE: This relies on the maxID being ~0u, so adding 1 brings us back to 0u. */
        baseId = m_mapData.getMaxId().asUint32() + 1u;
        basePosition = m_mapData.getLrb();
        if (basePosition.x + basePosition.y + basePosition.z != 0) {
            //basePosition.y++;
            //basePosition.x = 0;
            //basePosition.z = 0;
            basePosition.y = 0;
            basePosition.x = 0;
            basePosition.z = -1;
        }

        emit log("MapStorage", "Loading data ...");

        auto &progressCounter = getProgressCounter();
        progressCounter.reset();

        QDataStream stream(m_file);
        auto helper = LoadRoomHelperBase{stream};
        m_mapData.setDataChanged();

        // Read the version and magic
        static constexpr const auto MMAPPER_MAGIC = static_cast<int32_t>(0xFFB2AF01u);
        if (helper.read_i32() != MMAPPER_MAGIC) {
            return false;
        }
        const auto version = helper.read_u32();
        if (version != MMAPPER_2_5_1_SCHEMA && version != MMAPPER_2_4_3_SCHEMA
            && version != MMAPPER_2_4_0_SCHEMA && version != MMAPPER_2_3_7_SCHEMA
            && version != MMAPPER_2_0_4_SCHEMA && version != MMAPPER_2_0_2_SCHEMA
            && version != MMAPPER_1_1_0_SCHEMA && version != MMAPPER_2_0_0_SCHEMA
            && version != MMAPPER_1_0_0_SCHEMA) {
            const bool isNewer = version >= CURRENT_SCHEMA;
            QMessageBox::critical(dynamic_cast<QWidget *>(parent()),
                                  "MapStorage Error",
                                  QString("This map has schema version %1 which is too %2.\r\n\r\n"
                                          "Please %3 MMapper.")
                                      .arg(version)
                                      .arg(isNewer ? "new" : "old")
                                      .arg(isNewer ? "upgrade to the latest"
                                                   : "try an older version of"));
            return false;
        }

        // Force serialization to Qt4.8 because Qt5 has broke backwards compatability with QDateTime serialization
        // http://doc.qt.io/qt-5/sourcebreaks.html#changes-to-qdate-qtime-and-qdatetime
        // http://doc.qt.io/qt-5/qdatastream.html#versioning
        stream.setVersion(QDataStream::Qt_4_8);

        /* Caution! Stream requires buffer to have extended lifetime for new version,
         * so don't be tempted to move this inside the scope. */
        QBuffer buffer{};
        if (version >= MMAPPER_2_4_3_SCHEMA) {
            QByteArray compressedData(stream.device()->readAll());
            QByteArray uncompressedData = qUncompress(compressedData);
            buffer.setData(uncompressedData);
            buffer.open(QIODevice::ReadOnly);
            stream.setDevice(&buffer);
            emit log("MapStorage", "Uncompressed data");

        } else if (version >= MMAPPER_2_0_4_SCHEMA && version <= MMAPPER_2_4_0_SCHEMA) {
            /* once we require c++17, this can use if constexpr(USE_IO_COMPRESSOR) */
            if (USE_IO_COMPRESSOR) {
                auto *compressor = new QtIOCompressor(m_file);
                compressor->open(QIODevice::ReadOnly);
                stream.setDevice(compressor);
            } else {
                QMessageBox::critical(
                    dynamic_cast<QWidget *>(parent()),
                    "MapStorage Error",
                    "MMapper could not load this map because it is too old.\r\n\r\n"
                    "Please recompile MMapper with QtIOCompressor support and try again.");
                return false;
            }
        }
        emit log("MapStorage", QString("Schema version: %1").arg(version));

        const quint32 roomsCount = helper.read_u32();
        const quint32 connectionsCount = (version < MMAPPER_1_1_0_SCHEMA) ? helper.read_u32() : 0u;
        const quint32 marksCount = helper.read_u32();

        progressCounter.increaseTotalStepsBy(roomsCount + connectionsCount + marksCount);

        {
            Coordinate pos{};
            pos.x = helper.read_i32();
            pos.y = helper.read_i32();
            pos.z = (version < MMAPPER_1_1_0_SCHEMA) ? 0u : helper.read_i32();
            pos += basePosition;
            m_mapData.setPosition(pos);
        }

        emit log("MapStorage", QString("Number of rooms: %1").arg(roomsCount));

        ConnectionList connectionList{};
        // create all pointers to connections
        for (quint32 index = 0; index < connectionsCount; ++index) {
            connectionList.append(new Connection);
        }

        RoomVector roomList(roomsCount);
        for (uint i = 0; i < roomsCount; ++i) {
            Room *room = nullptr;
            if (version < MMAPPER_1_1_0_SCHEMA) { // OLD VERSIONS SUPPORT CODE
                room = loadOldRoom(stream, connectionList);
                room->setId(RoomId{baseId + i});
                roomList[i] = room;
            } else {
                room = loadRoom(stream, version);
            }

            progressCounter.step();
            m_mapData.insertPredefinedRoom(*room);
        }

        if (version < MMAPPER_1_1_0_SCHEMA) {
            emit log("MapStorage", QString("Number of connections: %1").arg(connectionsCount));
            ConnectionListIterator c(connectionList);
            while (c.hasNext()) {
                Connection *connection = c.next();
                loadOldConnection(connection, stream, roomList);
                translateOldConnection(connection);
                delete connection;

                progressCounter.step();
            }
            connectionList.clear();
        }

        emit log("MapStorage", QString("Number of info items: %1").arg(marksCount));

        MarkerList &markerList = m_mapData.getMarkersList();
        // create all pointers to items
        for (quint32 index = 0; index < marksCount; ++index) {
            auto mark = new InfoMark();
            loadMark(mark, stream, version);
            markerList.append(mark);

            progressCounter.step();
        }

        emit log("MapStorage", "Finished loading.");
        buffer.close();
        m_file->close();

        if (USE_IO_COMPRESSOR && version >= MMAPPER_2_0_4_SCHEMA
            && version <= MMAPPER_2_4_0_SCHEMA) {
            auto *compressor = dynamic_cast<QtIOCompressor *>(stream.device());
            compressor->close();
            delete compressor;
        }

        if (m_mapData.getRoomsCount() == 0u) {
            return false;
        }

        m_mapData.setFileName(m_fileName);
        m_mapData.unsetDataChanged();
    }

    m_mapData.checkSize();
    emit onDataLoaded();
    return true;
}

void MapStorage::loadMark(InfoMark *mark, QDataStream &stream, qint32 version)
{
    auto helper = LoadRoomHelperBase{stream};
    const qint32 postfix = basePosition.x + basePosition.y + basePosition.z;

    if (version < MMAPPER_1_1_0_SCHEMA) { // OLD VERSIONS SUPPORT CODE
        {
            auto name = helper.read_string();
            if (postfix != 0 && postfix != 1) {
                name += QString("_m%1").arg(postfix);
            }
            mark->setName(name);
        }
        mark->setText(helper.read_string());
        mark->setType(static_cast<InfoMarkType>(helper.read_u16()));

        auto read_pos = [](auto &helper, auto &basePosition) {
            Coordinate pos;
            pos.x = helper.read_i32();
            pos.x = pos.x * 100 / 48 - 40;
            pos.y = helper.read_i32();
            pos.y = pos.y * 100 / 48 - 55;
            //pos += basePosition;
            pos.x += basePosition.x * 100;
            pos.y += basePosition.y * 100;
            pos.z += basePosition.z;
            return pos;
        };
        mark->setPosition1(read_pos(helper, basePosition));
        mark->setPosition2(read_pos(helper, basePosition));

        mark->setRotationAngle(0.0);
    } else {
        {
            auto name = helper.read_string();
            if (postfix != 0 && postfix != 1) {
                name += QString("_m%1").arg(postfix).toLatin1();
            }
            mark->setName(name);
        }

        mark->setText(helper.read_string());
        mark->setTimeStamp(helper.read_datetime());

        mark->setType(static_cast<InfoMarkType>(helper.read_u8()));
        if (version >= MMAPPER_2_3_7_SCHEMA) {
            mark->setClass(static_cast<InfoMarkClass>(helper.read_u8()));
            /* REVISIT: was this intended to be divided by 100.0? */
            mark->setRotationAngle(helper.read_u32() / 100);
        }

        auto read_coord = [](auto &helper, auto &basePosition) {
            Coordinate pos;
            pos.x = helper.read_i32();
            pos.y = helper.read_i32();
            pos.z = helper.read_i32();
            pos.x += basePosition.x * 100;
            pos.y += basePosition.y * 100;
            pos.z += basePosition.z;
            return pos;
        };

        mark->setPosition1(read_coord(helper, basePosition));
        mark->setPosition2(read_coord(helper, basePosition));
    }
}

static DoorFlags sanitizeOldDoorFlags(const OldDoorFlags oldDoorFlags)
{
    static constexpr const uint32_t MASK = (1u << NUM_OLD_DOOR_FLAGS) - 1u;
    static_assert(MASK == 0x3Fu, "");
    return static_cast<DoorFlags>(oldDoorFlags.asUint32() & MASK);
}

static void setDoorNameAndFlags(Exit &e, const Door &door)
{
    e.setDoorName(door.getName());
    e.setDoorFlags(sanitizeOldDoorFlags(door.getFlags()));
}

void MapStorage::translateOldConnection(Connection *c)
{
    Room *left = c->getRoom(Hand::LEFT);
    Room *right = c->getRoom(Hand::RIGHT);
    ConnectionDirection leftDir = c->getDirection(Hand::LEFT);
    ConnectionDirection rightDir = c->getDirection(Hand::RIGHT);
    ConnectionFlags cFlags = c->getFlags();

    if (leftDir != ConnectionDirection::NONE) {
        Exit &e = left->exit(leftDir);
        e.addOut(right->getId());
        ExitFlags eFlags = e.getExitFlags();
        if (cFlags.contains(ConnectionFlag::DOOR)) {
            eFlags |= ExitFlag::NO_MATCH;
            eFlags |= ExitFlag::DOOR;
            const auto leftDoor = c->getDoor(Hand::LEFT);
            setDoorNameAndFlags(e, *leftDoor);
        }
        if (cFlags.contains(ConnectionFlag::RANDOM)) {
            eFlags |= ExitFlag::RANDOM;
        }
        if (cFlags.contains(ConnectionFlag::CLIMB)) {
            eFlags |= ExitFlag::CLIMB;
        }
        if (cFlags.contains(ConnectionFlag::SPECIAL)) {
            eFlags |= ExitFlag::SPECIAL;
        }
        eFlags |= ExitFlag::EXIT;
        e.setExitFlags(eFlags);

        Exit &eR = right->exit(opposite(leftDir));
        eR.addIn(left->getId());
    }
    if (rightDir != ConnectionDirection::NONE) {
        Exit &eL = left->exit(opposite(rightDir));
        eL.addIn(right->getId());

        Exit &e = right->exit(rightDir);
        e.addOut(left->getId());
        ExitFlags eFlags = e.getExitFlags();
        if (cFlags.contains(ConnectionFlag::DOOR)) {
            eFlags |= ExitFlag::DOOR;
            eFlags |= ExitFlag::NO_MATCH;
            const auto rightDoor = c->getDoor(Hand::RIGHT);
            setDoorNameAndFlags(e, *rightDoor);
        }
        if (cFlags.contains(ConnectionFlag::RANDOM)) {
            eFlags |= ExitFlag::RANDOM;
        }
        if (cFlags.contains(ConnectionFlag::CLIMB)) {
            eFlags |= ExitFlag::CLIMB;
        }
        if (cFlags.contains(ConnectionFlag::SPECIAL)) {
            eFlags |= ExitFlag::SPECIAL;
        }
        eFlags |= ExitFlag::EXIT;
        e.setExitFlags(eFlags);
    }
}

// TODO: use gsl::narrow_cast<>
template<typename To, typename From>
To narrow_cast(const From from)
{
    static_assert(std::is_integral<From>::value, "");
    static_assert(std::is_integral<To>::value, "");
    static_assert(sizeof(To) <= sizeof(From), "");

    const auto to = static_cast<To>(from);
    if (std::is_signed<From>::value && !std::is_signed<To>::value) {
        assert(from >= 0);
    }
    if (!std::is_signed<From>::value && std::is_signed<To>::value) {
        assert(to >= 0);
    }
    assert(static_cast<From>(to) == from);
    return to;
}

template<typename To, typename From>
To widen_cast(const From from)
{
    static_assert(std::is_integral<From>::value, "");
    static_assert(std::is_integral<To>::value, "");
    static_assert(sizeof(To) >= sizeof(From), "");

    const auto to = static_cast<To>(from);
    if (std::is_signed<From>::value && !std::is_signed<To>::value) {
        assert(from >= 0);
    }
    if (!std::is_signed<From>::value && std::is_signed<To>::value) {
        assert(to >= 0);
    }
    assert(static_cast<From>(to) == from);
    return to;
}

void MapStorage::loadOldConnection(Connection *connection,
                                   QDataStream &istream,
                                   RoomVector &roomList)
{
    auto helper = OldLoadRoomHelper{istream};

    connection->setNote("");

    const auto decodeCtCf = [](uint16_t ctcf) {
        return std::make_pair(static_cast<ConnectionType>(widen_cast<int>(ctcf & 0x3u)),
                              static_cast<ConnectionFlags>(narrow_cast<uint8_t>(ctcf >> 2)));
    };
    const auto decodeDoorFlags = [](uint16_t df) -> DoorFlags {
        DoorFlags result{};
        if (df & 0x1)
            result |= DoorFlag::HIDDEN;
        if (df & 0x2)
            result |= DoorFlag::NEED_KEY;
        return result;
    };

    const auto ctcf = decodeCtCf(helper.read_u16());
    ConnectionType ct = ctcf.first;
    const ConnectionFlags cf = ctcf.second;

    const auto doorFlags1 = decodeDoorFlags(helper.read_u16());
    const auto doorFlags2 = decodeDoorFlags(helper.read_u16());

    const DoorName doorName1 = helper.read_string();
    const DoorName doorName2 = helper.read_string();

    // REVISIT: just throw here if it fails?
    const auto decodeRoom = [&](const uint32_t roomIdPlus1) -> Room * {
        if (roomIdPlus1 == 0u)
            return nullptr;
        const auto roomId = roomIdPlus1 - 1u;
        if (roomId >= static_cast<uint>(roomList.size()))
            return nullptr;
        return roomList[roomId];
    };

    Room *r1 = decodeRoom(helper.read_u32());
    Room *r2 = decodeRoom(helper.read_u32());

    // BTW, asserting here won't actually do anything on a release build.
    // You need to throw an exception if this happens, or the mapper will crash.
    // (it'll also crash if you don't catch the exception).
#define parse_assert(x) \
    do { \
        if (x) \
            continue; \
        throw std::runtime_error("assertion failure: " #x); \
    } while (false)

    parse_assert(r1 != nullptr); // if these are not valid,
    parse_assert(r2 != nullptr); // the indices were out of bounds

    if (cf.contains(ConnectionFlag::DOOR)) {
        /* REVISIT: This might be slicing off important bits by casting to OldDoorFlags,
         * but we're loading an old save file, so maybe it's okay. */
        connection->setDoor(new Door(doorName1,
                                     OldDoorFlags{narrow_cast<uint16_t>(doorFlags1.asUint32())}),
                            Hand::LEFT);
        connection->setDoor(new Door(doorName2,
                                     OldDoorFlags{narrow_cast<uint16_t>(doorFlags2.asUint32())}),
                            Hand::RIGHT);
    }

    if (connection->getRoom(Hand::LEFT) == nullptr) {
        auto room = connection->getRoom(Hand::RIGHT);
        connection->setRoom((room != r1) ? r1 : r2, Hand::LEFT);
    }
    if (connection->getRoom(Hand::RIGHT) == nullptr) {
        auto room = connection->getRoom(Hand::LEFT);
        connection->setRoom((room != r1) ? r1 : r2, Hand::RIGHT);
    }

    parse_assert(connection->getRoom(Hand::LEFT) != nullptr);
    parse_assert(connection->getRoom(Hand::RIGHT) != nullptr);

    if (cf.contains(ConnectionFlag::DOOR)) {
        parse_assert(connection->getDoor(Hand::LEFT) != nullptr);
        parse_assert(connection->getDoor(Hand::RIGHT) != nullptr);
    }

    if (connection->getDirection(Hand::RIGHT) == ConnectionDirection::UNKNOWN) {
        ct = ConnectionType::ONE_WAY;
        connection->setDirection(ConnectionDirection::NONE, Hand::RIGHT);
    } else if (connection->getDirection(Hand::LEFT) == ConnectionDirection::UNKNOWN) {
        ct = ConnectionType::ONE_WAY;
        connection->setDirection(connection->getDirection(Hand::RIGHT), Hand::LEFT);
        connection->setDirection(ConnectionDirection::NONE, Hand::LEFT);
        Room *temp = connection->getRoom(Hand::LEFT);
        connection->setRoom(connection->getRoom(Hand::RIGHT), Hand::LEFT);
        connection->setRoom(temp, Hand::RIGHT);
    }

    if (connection->getRoom(Hand::LEFT) == connection->getRoom(Hand::RIGHT)) {
        ct = ConnectionType::LOOP;
    }

    connection->setType(ct);
    connection->setFlags(cf);
}

void MapStorage::saveRoom(const Room &room, QDataStream &stream)
{
    stream << room.getName();
    stream << room.getStaticDescription();
    stream << room.getDynamicDescription();
    stream << static_cast<quint32>(room.getId());
    stream << room.getNote();
    stream << static_cast<quint8>(room.getTerrainType());
    stream << static_cast<quint8>(room.getLightType());
    stream << static_cast<quint8>(room.getAlignType());
    stream << static_cast<quint8>(room.getPortableType());
    stream << static_cast<quint8>(room.getRidableType());
    stream << static_cast<quint8>(room.getSundeathType());
    stream << static_cast<quint32>(room.getMobFlags());
    stream << static_cast<quint32>(room.getLoadFlags());
    stream << static_cast<quint8>(room.isUpToDate());

    const Coordinate &pos = room.getPosition();
    stream << static_cast<qint32>(pos.x);
    stream << static_cast<qint32>(pos.y);
    stream << static_cast<qint32>(pos.z);
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
        stream << e.getDoorName();
        for (auto idx : e.inRange()) {
            stream << static_cast<quint32>(idx);
        }
        stream << UINT_MAX;
        for (auto idx : e.outRange()) {
            stream << static_cast<quint32>(idx);
        }
        stream << UINT_MAX;
    }
}

bool MapStorage::saveData(bool baseMapOnly)
{
    emit log("MapStorage", "Writing data to file ...");

    QDataStream fileStream(m_file);
    fileStream.setVersion(QDataStream::Qt_4_8);

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    ConstRoomList roomList;
    MarkerList &markerList = m_mapData.getMarkersList();
    RoomSaver saver(m_mapData, roomList);
    for (uint i = 0; i < m_mapData.getRoomsCount(); ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }

    uint roomsCount = saver.getRoomsCount();
    uint marksCount = markerList.size();

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

    //write counters
    stream << static_cast<quint32>(roomsCount);
    stream << static_cast<quint32>(marksCount);

    //write selected room x,y
    Coordinate &self = m_mapData.getPosition();
    stream << static_cast<qint32>(self.x);
    stream << static_cast<qint32>(self.y);
    stream << static_cast<qint32>(self.z);

    // save rooms
    QListIterator<const Room *> roomit(roomList);
    while (roomit.hasNext()) {
        const Room *const pRoom = roomit.next();
        const Room &room = deref(pRoom);

        if (baseMapOnly) {
            BaseMapSaveFilter::Action action = filter.filter(room);
            if (!room.isTemporary() && action != BaseMapSaveFilter::Action::REJECT) {
                if (action == BaseMapSaveFilter::Action::ALTER) {
                    Room copy = filter.alteredRoom(room);
                    saveRoom(copy, stream);
                } else { // action == PASS
                    saveRoom(room, stream);
                }
            }
        } else {
            saveRoom(room, stream);
        }

        progressCounter.step();
    }

    // save items
    MarkerListIterator markerit(markerList);
    while (markerit.hasNext()) {
        InfoMark *mark = markerit.next();
        saveMark(mark, stream);

        progressCounter.step();
    }

    buffer.close();

    QByteArray uncompressedData(buffer.data());
    QByteArray compressedData = qCompress(uncompressedData);
    progressCounter.step();
    double compressionRatio = (compressedData.size() == 0)
                                  ? 1.0
                                  : (static_cast<double>(uncompressedData.size())
                                     / static_cast<double>(compressedData.size()));
    emit log("MapStorage",
             QString("Map compressed (compression ratio of %1:1)")
                 .arg(QString::number(compressionRatio, 'f', 1)));

    fileStream.writeRawData(compressedData.data(), compressedData.size());
    emit log("MapStorage", "Writing data finished.");

    m_mapData.unsetDataChanged();
    emit onDataSaved();

    return true;
}

void MapStorage::saveMark(InfoMark *mark, QDataStream &stream)
{
    stream << QString(mark->getName());
    stream << QString(mark->getText());
    stream << QDateTime(mark->getTimeStamp());
    stream << static_cast<quint8>(mark->getType());
    stream << static_cast<quint8>(mark->getClass());
    stream << static_cast<qint32>(mark->getRotationAngle() * 100);
    const Coordinate &c1 = mark->getPosition1();
    const Coordinate &c2 = mark->getPosition2();
    stream << static_cast<qint32>(c1.x);
    stream << static_cast<qint32>(c1.y);
    stream << static_cast<qint32>(c1.z);
    stream << static_cast<qint32>(c2.x);
    stream << static_cast<qint32>(c2.y);
    stream << static_cast<qint32>(c2.z);
}
