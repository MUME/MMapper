// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "jsonmapstorage.h"

#include "../global/Charset.h"
#include "../global/parserutils.h"
#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFlags.h"
#include "../map/coordinate.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "abstractmapstorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>

#include <QString>

namespace { // anonymous

// These settings have to be shared with the JS code:
// Group all rooms with the same 2 first hash bytes into the same file
static constexpr const int c_roomIndexFileNameSize = 2;
// Split the world into 20x20 zones
static constexpr const int ZONE_WIDTH = 20;

/* Performs MD5 hashing on ASCII-transliterated, whitespace-normalized name+descs.
 * MD5 is for convenience (easily available in all languages), the rest makes
 * the hash resilient to trivial typo fixes by the builders.
 */
class NODISCARD WebHasher final
{
    QCryptographicHash m_hash;

public:
    WebHasher()
        : m_hash(QCryptographicHash::Md5)
    {}

    void add(const RoomName &roomName, const RoomDesc &roomDesc)
    {
        auto name = roomName.toQString();
        // This is most likely unnecessary because the parser did it for us...
        // We need plain ASCII so that accentuation changes do not affect the
        // hashes and because MD5 is defined on bytes, not encoded chars.
        mmqt::toAsciiInPlace(name);
        // Roomdescs may see whitespacing fixes over the years (ex: removing double
        // spaces after periods). MMapper ignores such changes when comparing rooms,
        // but the web mapper may only look up rooms by hash. Normalizing the
        // whitespaces makes the hash more resilient.
        auto desc = mmqt::toQStringUtf8(
            ParserUtils::normalizeWhitespace(roomDesc.toStdStringUtf8()));
        mmqt::toAsciiInPlace(desc);

        m_hash.addData(name.toLatin1() + char_consts::C_NEWLINE + desc.toLatin1()); // ASCII
    }

    NODISCARD QByteArray result() const { return m_hash.result(); }

    void reset() { m_hash.reset(); }
};

// Lets the webclient locate and load the useful zones only, not the whole
// world at once.
class NODISCARD RoomHashIndex final
{
public:
    using Index = QMultiMap<QByteArray, Coordinate>;

private:
    Index m_index;
    WebHasher m_hasher;

public:
    void addRoom(const RoomHandle &room)
    {
        m_hasher.add(room.getName(), room.getDescription());
        m_index.insert(m_hasher.result().toHex(), room.getPosition());
        m_hasher.reset();
    }

    NODISCARD const Index &index() const { return m_index; }
};

NODISCARD static std::string getZoneKey(const Coordinate &c)
{
    static constexpr const auto calcZoneCoord = [](const int n) -> int {
        auto f = [](const int x) -> int { return (x / ZONE_WIDTH) * ZONE_WIDTH; };
        return f((n < 0) ? (n + 1 - ZONE_WIDTH) : n);
    };
    static_assert(calcZoneCoord(-ZONE_WIDTH - 1) == -2 * ZONE_WIDTH);
    static_assert(calcZoneCoord(-ZONE_WIDTH) == -ZONE_WIDTH);
    static_assert(calcZoneCoord(-1) == -ZONE_WIDTH);
    static_assert(calcZoneCoord(0) == 0);
    static_assert(calcZoneCoord(ZONE_WIDTH - 1) == 0);
    static_assert(calcZoneCoord(ZONE_WIDTH) == ZONE_WIDTH);

    // REVISIT: consider sprintf here instead of std::string concatenation if this shows up in perf.
    return std::to_string(calcZoneCoord(c.x)) + "," + std::to_string(calcZoneCoord(-c.y));
}

// Splits the world in zones easier to download and load
class NODISCARD ZoneIndex final
{
public:
    using Index = std::unordered_map<std::string, ConstRoomList>;

private:
    Index m_index;

public:
    void addRoom(const RoomHandle &room)
    {
        const auto zone = getZoneKey(room.getPosition());
        const auto iter = m_index.find(zone);
        if (iter == m_index.end()) {
            ConstRoomList list;
            list.emplace_back(room);
            m_index.emplace(zone, std::move(list));
        } else {
            iter->second.emplace_back(room);
        }
    }

    NODISCARD const Index &index() const { return m_index; }
};

template<typename JsonT>
static void writeJson(const QString &filePath, JsonT &json, const QString &what)
{
    static_assert(std::is_same_v<JsonT, QJsonObject> || std::is_same_v<JsonT, QJsonArray>);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QString msg(
            QString("error opening %1 to %2: %3").arg(what).arg(filePath).arg(file.errorString()));
        throw std::runtime_error(mmqt::toStdStringUtf8(msg));
    }

    QJsonDocument doc(json);
    QTextStream stream(&file);
    stream << doc.toJson();
    stream.flush();
    if (!file.flush() || stream.status() != QTextStream::Ok) {
        QString msg(
            QString("error writing %1 to %2: %3").arg(what).arg(filePath).arg(file.errorString()));
        throw std::runtime_error(mmqt::toStdStringUtf8(msg));
    }
}

class NODISCARD RoomIndexStore final
{
    const QDir m_dir;
    QJsonObject m_hashes;
    QByteArray m_prefix;

public:
    RoomIndexStore() = delete;

public:
    explicit RoomIndexStore(const QDir &dir)
        : m_dir(dir)
    {}

    void add(const QByteArray &hash, Coordinate coords)
    {
        QByteArray prefix = hash.left(c_roomIndexFileNameSize);
        if (m_prefix != prefix) {
            close();
            m_prefix = prefix;
        }

        QJsonArray jCoords;
        jCoords.push_back(coords.x);
        jCoords.push_back(coords.y * -1);
        jCoords.push_back(coords.z);

        QJsonObject::iterator collision = m_hashes.find(hash);
        if (collision == m_hashes.end()) {
            QJsonArray array;
            array.push_back(jCoords);
            m_hashes.insert(hash, array);
        } else {
            collision->toArray().push_back(jCoords);
        }
    }

    void close()
    {
        if (m_hashes.isEmpty()) {
            return;
        }

        assert(!m_prefix.isEmpty());
        QString filePath = m_dir.filePath(QString::fromLocal8Bit(m_prefix) + ".json");

        writeJson(filePath, m_hashes, "room index");

        m_hashes = QJsonObject();
        m_prefix = QByteArray();
    }
};

using JsonRoomId = uint32_t;

// Maps MM2 room IDs -> hole-free JSON room IDs
class NODISCARD JsonRoomIdsCache final
{
    using CacheT = QMap<ExternalRoomId, JsonRoomId>;
    CacheT m_cache;
    JsonRoomId m_nextJsonId = 0u;

public:
    JsonRoomIdsCache();
    void addRoom(const ExternalRoomId mm2RoomId) { m_cache[mm2RoomId] = m_nextJsonId++; }
    NODISCARD JsonRoomId operator[](ExternalRoomId roomId) const;
    NODISCARD uint32_t size() const;
};

JsonRoomIdsCache::JsonRoomIdsCache() = default;

JsonRoomId JsonRoomIdsCache::operator[](const ExternalRoomId roomId) const
{
    CacheT::const_iterator it = m_cache.find(roomId);
    assert(it != m_cache.end());
    return *it;
}

uint32_t JsonRoomIdsCache::size() const
{
    return m_nextJsonId;
}

// Expects that a RoomSaver locks the Rooms for the lifetime of this object!
class NODISCARD JsonWorld final
{
    JsonRoomIdsCache m_jRoomIds;
    RoomHashIndex m_roomHashIndex;
    ZoneIndex m_zoneIndex;

    void addRoom(QJsonArray &jRooms, const ExternalRawRoom &room) const;
    void addExits(const ExternalRawRoom &room, QJsonObject &jr) const;

public:
    JsonWorld();
    ~JsonWorld();
    void addRooms(const ConstRoomList &roomList, ProgressCounter &progressCounter);
    void writeMetadata(const QFileInfo &path, const Bounds &bounds) const;
    void writeRoomIndex(const QDir &dir) const;
    void writeZones(const QDir &dir, ProgressCounter &progressCounter) const;
};

JsonWorld::JsonWorld() = default;

JsonWorld::~JsonWorld() = default;

void JsonWorld::addRooms(const ConstRoomList &roomList, ProgressCounter &progressCounter)
{
    for (const auto &pRoom : roomList) {
        const auto &room = deref(pRoom);
        progressCounter.step();
        m_jRoomIds.addRoom(room.getIdExternal());
        m_roomHashIndex.addRoom(room);
        m_zoneIndex.addRoom(room);
    }
}

NODISCARD static constexpr const char *getNameUpper(const ExitDirEnum dir)
{
#define CASE(x) \
    case ExitDirEnum::x: \
        return #x
    switch (dir) {
        CASE(NORTH);
        CASE(SOUTH);
        CASE(EAST);
        CASE(WEST);
        CASE(UP);
        CASE(DOWN);
        CASE(UNKNOWN);
        CASE(NONE);

    /* Please keep this here in case invalid type is cast to ExitDirEnum.
    * Consider changing it to an assertion, throw, or abort(). */
    default:
        return "*ERROR*";
    }
#undef CASE
}

void JsonWorld::writeMetadata(const QFileInfo &path, const Bounds &bounds) const
{
    // This can give bogus data if the bounds aren't set.
    const Coordinate &min = bounds.min;
    const Coordinate &max = bounds.max;

    QJsonObject meta;
    meta["roomsCount"] = static_cast<qint64>(m_jRoomIds.size());
    meta["minX"] = min.x;
    meta["minY"] = std::min(-min.y, -max.y);
    meta["minZ"] = min.z;
    meta["maxX"] = max.x;
    meta["maxY"] = std::max(-min.y, -max.y);
    meta["maxZ"] = max.z;

    meta["directions"] = []() {
        QJsonArray arr;
        // why is there no QJsonArray::resize()?
        for (size_t i = 0; i <= NUM_EXITS; ++i) {
            arr.push_back(QString{});
        }
        for (size_t i = 0; i <= NUM_EXITS; ++i) {
            arr[static_cast<int>(i)] = getNameUpper(static_cast<ExitDirEnum>(i));
        }
        return arr;
    }();

    writeJson(path.filePath(), meta, "metadata");
}

void JsonWorld::writeRoomIndex(const QDir &dir) const
{
    using IterT = RoomHashIndex::Index::const_iterator;
    const RoomHashIndex::Index &index = m_roomHashIndex.index();

    RoomIndexStore store(dir);
    for (IterT iter = index.cbegin(); iter != index.cend(); ++iter) {
        store.add(iter.key(), iter.value());
    }
    store.close();
}

void JsonWorld::addRoom(QJsonArray &jRooms, const ExternalRawRoom &room) const
{
    /*
          x: 5, y: 5, z: 0,
          north: null, east: 1, south: null, west: null, up: null, down: null,
          sector: 2 * SECT_CITY *, mobflags: 0, loadflags: 0, light: null, RIDEABLE: null,
          name: "Fortune's Delving",
          desc:
          "A largely ceremonial hall, it was the first mineshaft that led down to what is\n"
    */

    const Coordinate &pos = room.getPosition();
    QJsonObject jr;
    jr["x"] = pos.x;
    jr["y"] = -pos.y;
    jr["z"] = pos.z;

    const uint32_t jsonId = m_jRoomIds[room.getId()];
    jr["id"] = QString::number(jsonId);
    jr["name"] = room.getName().toQString();
    jr["desc"] = room.getDescription().toQString();
    jr["sector"] = static_cast<uint8_t>(room.getTerrainType());
    jr["light"] = static_cast<uint8_t>(room.getLightType());
    jr["portable"] = static_cast<uint8_t>(room.getPortableType());
    jr["rideable"] = static_cast<uint8_t>(room.getRidableType());
    jr["sundeath"] = static_cast<uint8_t>(room.getSundeathType());
    jr["mobflags"] = static_cast<qint64>(room.getMobFlags().asUint32());
    jr["loadflags"] = static_cast<qint64>(room.getLoadFlags().asUint32());

    addExits(room, jr);

    jRooms.push_back(jr);
}

void JsonWorld::addExits(const ExternalRawRoom &room, QJsonObject &jr) const
{
    QJsonArray jExits; // Direction-indexed
    for (const ExternalRawExit &e : room.exits) {
        QJsonObject je;
        je["flags"] = static_cast<qint64>(e.getExitFlags().asUint32());
        je["dflags"] = static_cast<qint64>(e.getDoorFlags().asUint32());
        je["name"] = e.getDoorName().toQString();

        QJsonArray jin;
        for (const ExternalRoomId idx : e.getIncomingSet()) {
            jin << QString::number(m_jRoomIds[idx]);
        }
        je["in"] = jin;

        QJsonArray jout;
        for (const ExternalRoomId idx : e.getOutgoingSet()) {
            jout << QString::number(m_jRoomIds[idx]);
        }
        je["out"] = jout;

        jExits << je;
    }

    jr["exits"] = jExits;
}

void JsonWorld::writeZones(const QDir &dir, ProgressCounter &progressCounter) const
{
    const ZoneIndex::Index &index = m_zoneIndex.index();

    for (const auto &kv : index) {
        const ConstRoomList &rooms = kv.second;
        QJsonArray jRooms;
        for (const RoomPtr &pRoom : rooms) {
            addRoom(jRooms, deref(pRoom).getRawCopyExternal());
            progressCounter.step();
        }

        QString filePath = dir.filePath(mmqt::toQStringUtf8(kv.first + ".json"));
        writeJson(filePath, jRooms, "zone");
    }
}

} // namespace

JsonMapStorage::JsonMapStorage(const AbstractMapStorage::Data &data, QObject *parent)
    : AbstractMapStorage{data, parent}
{}

JsonMapStorage::~JsonMapStorage() = default;

bool JsonMapStorage::virt_saveData(const RawMapData &mapData)
{
    log("Writing data to files ...");

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    ConstRoomList roomList;

    const auto &map = mapData.mapPair.modified;
    const RawMarkerData noMarkers;
    const RawMarkerData &markerList = mapData.markerData.has_value() ? mapData.markerData.value()
                                                                     : noMarkers;

    // REVISIT: This only excludes temporary rooms from being written,
    // but it doesn't exclude temporary rooms from connections.
    roomList.reserve(map.getRoomsCount());
    for (const RoomId id : map.getRooms()) {
        const RoomHandle &room = map.getRoomHandle(id);
        if (!room.isTemporary()) {
            roomList.push_back(room);
        }
    }

    const auto roomsCount = static_cast<uint32_t>(roomList.size());
    const auto marksCount = static_cast<uint32_t>(markerList.size());

    auto &progressCounter = getProgressCounter();
    progressCounter.setNewTask(ProgressMsg{}, roomsCount * 2 + marksCount);

    JsonWorld world;
    world.addRooms(roomList, progressCounter);

    QDir saveDir(getFilename());
    QDir destDir(QFileInfo(saveDir, "v1").filePath());
    QDir roomIndexDir(QFileInfo(destDir, "roomindex").filePath());
    QDir zoneDir(QFileInfo(destDir, "zone").filePath());
    try {
        QDir dir;
        if (!dir.mkpath(destDir.path())) {
            throw std::runtime_error("error creating dir v1");
        }
        if (!dir.mkpath(roomIndexDir.path())) {
            throw std::runtime_error("error creating dir v1/roomindex");
        }
        if (!dir.mkpath(zoneDir.path())) {
            throw std::runtime_error("error creating dir v1/zone");
        }

        world.writeMetadata(QFileInfo(destDir, "arda.json"), map.getBounds().value_or(Bounds{}));
        world.writeRoomIndex(roomIndexDir);
        world.writeZones(zoneDir, progressCounter);
    } catch (const std::exception &e) {
        log(e.what());
        return false;
    }

    log("Writing data finished.");

    return true;
}
