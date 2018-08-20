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

static constexpr const int MMAPPER_2_0_0_SCHEMA = 17; // Initial schema
static constexpr const int MMAPPER_2_0_2_SCHEMA = 24; // Ridable flag
static constexpr const int MMAPPER_2_0_4_SCHEMA = 25; // QtIOCompressor
static constexpr const int MMAPPER_2_3_7_SCHEMA = 32; // 16bit DoorFlags, NoMatch
static constexpr const int MMAPPER_2_4_0_SCHEMA = 33; // 16bit ExitsFlags, 32bit MobFlags/LoadFlags
static constexpr const int MMAPPER_2_4_3_SCHEMA = 34; // qCompress, SunDeath flag
static constexpr const int MMAPPER_2_5_1_SCHEMA = 35; // discard all previous NoMatch flags
static constexpr const int CURRENT_SCHEMA = MMAPPER_2_5_1_SCHEMA;

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

class LoadRoomHelper final
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

Room *MapStorage::loadRoom(QDataStream &stream, uint32_t version)
{
    auto helper = LoadRoomHelper{stream};
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

void MapStorage::loadExits(Room &room, QDataStream &stream, const uint32_t version)
{
    auto helper = LoadRoomHelper{stream};

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
        auto helper = LoadRoomHelper{stream};
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
            && version != MMAPPER_2_0_0_SCHEMA) {
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

        const uint32_t roomsCount = helper.read_u32();
        const uint32_t marksCount = helper.read_u32();
        progressCounter.increaseTotalStepsBy(roomsCount + marksCount);

        {
            Coordinate pos{};
            pos.x = helper.read_i32();
            pos.y = helper.read_i32();
            pos.z = helper.read_i32();
            pos += basePosition;
            m_mapData.setPosition(pos);
        }

        emit log("MapStorage", QString("Number of rooms: %1").arg(roomsCount));

        for (uint32_t i = 0; i < roomsCount; ++i) {
            Room *room = loadRoom(stream, version);

            progressCounter.step();
            m_mapData.insertPredefinedRoom(*room);
        }

        emit log("MapStorage", QString("Number of info items: %1").arg(marksCount));

        MarkerList &markerList = m_mapData.getMarkersList();
        // create all pointers to items
        for (uint32_t index = 0; index < marksCount; ++index) {
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

void MapStorage::loadMark(InfoMark *mark, QDataStream &stream, uint32_t version)
{
    auto helper = LoadRoomHelper{stream};
    const qint32 postfix = basePosition.x + basePosition.y + basePosition.z;
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
