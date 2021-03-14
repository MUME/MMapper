// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "jsonmapstorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <QString>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../parser/parserutils.h"
#include "abstractmapstorage.h"
#include "basemapsavefilter.h"
#include "progresscounter.h"
#include "roomsaver.h"

namespace {

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

    void add(QString str)
    {
        // This is most likely unnecessary because the parser did it for us...
        // We need plain ASCII so that accentuation changes do not affect the
        // hashes and because MD5 is defined on bytes, not encoded chars.
        ParserUtils::toAsciiInPlace(str);
        // Roomdescs may see whitespacing fixes over the years (ex: removing double
        // spaces after periods). MMapper ignores such changes when comparing rooms,
        // but the web mapper may only look up rooms by hash. Normalizing the
        // whitespaces makes the hash more resilient.
        str.replace(QRegularExpression(" +"), " ");
        str.replace(QRegularExpression(" *\r?\n"), "\n");

        // REVISIT: should this be latin1 or utf8?
        m_hash.addData(str.toLatin1());
    }

    QByteArray result() const { return m_hash.result(); }

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
    void addRoom(const Room &room)
    {
        m_hasher.add(room.getName().toQString() + "\n");
        m_hasher.add(room.getDescription().toQString());
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
    void addRoom(const Room &room)
    {
        const auto zone = getZoneKey(room.getPosition());
        const auto iter = m_index.find(zone);
        if (iter == m_index.end()) {
            ConstRoomList list;
            list.emplace_back(room.shared_from_this());
            m_index.emplace(zone, std::move(list));
        } else {
            iter->second.emplace_back(room.shared_from_this());
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
        throw std::runtime_error(::toStdStringUtf8(msg));
    }

    QJsonDocument doc(json);
    QTextStream stream(&file);
    stream << doc.toJson();
    stream.flush();
    if (!file.flush() || stream.status() != QTextStream::Ok) {
        QString msg(
            QString("error writing %1 to %2: %3").arg(what).arg(filePath).arg(file.errorString()));
        throw std::runtime_error(::toStdStringUtf8(msg));
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

using JsonRoomId = uint;

// Maps MM2 room IDs -> hole-free JSON room IDs
class JsonRoomIdsCache
{
    using CacheT = QMap<RoomId, JsonRoomId>;
    CacheT m_cache;
    JsonRoomId m_nextJsonId = 0u;

public:
    JsonRoomIdsCache();
    void addRoom(RoomId mm2RoomId) { m_cache[mm2RoomId] = m_nextJsonId++; }
    JsonRoomId operator[](RoomId roomId) const;
    uint size() const;
};

JsonRoomIdsCache::JsonRoomIdsCache() = default;

JsonRoomId JsonRoomIdsCache::operator[](RoomId roomId) const
{
    CacheT::const_iterator it = m_cache.find(roomId);
    assert(it != m_cache.end());
    return *it;
}

uint JsonRoomIdsCache::size() const
{
    return m_nextJsonId;
}

// Expects that a RoomSaver locks the Rooms for the lifetime of this object!
class JsonWorld final
{
    JsonRoomIdsCache m_jRoomIds;
    RoomHashIndex m_roomHashIndex;
    ZoneIndex m_zoneIndex;

    void addRoom(QJsonArray &jRooms, const Room &room) const;
    void addExits(const Room &room, QJsonObject &jr) const;

public:
    JsonWorld();
    ~JsonWorld();
    void addRooms(const ConstRoomList &roomList,
                  BaseMapSaveFilter &filter,
                  ProgressCounter &progressCounter,
                  bool baseMapOnly);
    void writeMetadata(const QFileInfo &path, const MapData &mapData) const;
    void writeRoomIndex(const QDir &dir) const;
    void writeZones(const QDir &dir,
                    BaseMapSaveFilter &filter,
                    ProgressCounter &progressCounter,
                    bool baseMapOnly) const;
};

JsonWorld::JsonWorld() = default;

JsonWorld::~JsonWorld() = default;

void JsonWorld::addRooms(const ConstRoomList &roomList,
                         BaseMapSaveFilter &filter,
                         ProgressCounter &progressCounter,
                         bool baseMapOnly)
{
    for (const SharedConstRoom &pRoom : roomList) {
        const Room &room = deref(pRoom);
        progressCounter.step();

        if (baseMapOnly) {
            BaseMapSaveFilter::ActionEnum action = filter.filter(room);
            if (room.isTemporary() || action == BaseMapSaveFilter::ActionEnum::REJECT) {
                continue;
            }
        }

        m_jRoomIds.addRoom(room.getId());
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

void JsonWorld::writeMetadata(const QFileInfo &path, const MapData &mapData) const
{
    // This can give bogus data if the bounds aren't set.
    const Coordinate &min = mapData.getMin();
    const Coordinate &max = mapData.getMax();

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
        for (size_t i = 0; i <= NUM_EXITS; ++i)
            arr.push_back(QString{});
        for (size_t i = 0; i <= NUM_EXITS; ++i)
            arr[static_cast<int>(i)] = getNameUpper(static_cast<ExitDirEnum>(i));
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

void JsonWorld::addRoom(QJsonArray &jRooms, const Room &room) const
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

    uint jsonId = m_jRoomIds[room.getId()];
    jr["id"] = QString::number(jsonId);
    jr["name"] = room.getName().toQString();
    jr["desc"] = room.getDescription().toQString();
    jr["sector"] = static_cast<quint8>(room.getTerrainType());
    jr["light"] = static_cast<quint8>(room.getLightType());
    jr["portable"] = static_cast<quint8>(room.getPortableType());
    jr["rideable"] = static_cast<quint8>(room.getRidableType());
    jr["sundeath"] = static_cast<quint8>(room.getSundeathType());
    jr["mobflags"] = static_cast<qint64>(room.getMobFlags().asUint32());
    jr["loadflags"] = static_cast<qint64>(room.getLoadFlags().asUint32());

    addExits(room, jr);

    jRooms.push_back(jr);
}

void JsonWorld::addExits(const Room &room, QJsonObject &jr) const
{
    const ExitsList &exitList = room.getExitsList();
    QJsonArray jExits; // Direction-indexed
    for (const Exit &e : exitList) {
        QJsonObject je;
        je["flags"] = static_cast<qint64>(e.getExitFlags().asUint32());
        je["dflags"] = static_cast<qint64>(e.getDoorFlags().asUint32());
        je["name"] = e.getDoorName().toQString();

        QJsonArray jin;
        for (auto idx : e.inRange()) {
            jin << QString::number(m_jRoomIds[idx]);
        }
        je["in"] = jin;

        QJsonArray jout;
        for (auto idx : e.outRange()) {
            jout << QString::number(m_jRoomIds[idx]);
        }
        je["out"] = jout;

        jExits << je;
    }

    jr["exits"] = jExits;
}

void JsonWorld::writeZones(const QDir &dir,
                           BaseMapSaveFilter &filter,
                           ProgressCounter &progressCounter,
                           bool baseMapOnly) const
{
    const ZoneIndex::Index &index = m_zoneIndex.index();

    for (const auto &kv : index) {
        const ConstRoomList &rooms = kv.second;
        QJsonArray jRooms;
        auto saveOne = [this, &jRooms](const Room &room) { addRoom(jRooms, room); };
        for (const auto &pRoom : rooms) {
            filter.visitRoom(deref(pRoom), baseMapOnly, saveOne);
            progressCounter.step();
        }

        QString filePath = dir.filePath(::toQStringUtf8(kv.first + ".json"));
        writeJson(filePath, jRooms, "zone");
    }
}

} // namespace

JsonMapStorage::JsonMapStorage(MapData &mapdata, const QString &filename, QObject *parent)
    : AbstractMapStorage(mapdata, filename, parent)
{}

JsonMapStorage::~JsonMapStorage() = default;

void JsonMapStorage::newData()
{
    qWarning() << "JsonMapStorage does not implement newData()";
}

bool JsonMapStorage::loadData()
{
    return false;
}

bool JsonMapStorage::mergeData()
{
    return false;
}

bool JsonMapStorage::saveData(bool baseMapOnly)
{
    log("Writing data to files ...");

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    // The RoomSaver acts as a lock on the rooms.
    ConstRoomList roomList;
    roomList.reserve(m_mapData.getRoomsCount());

    const MarkerList &markerList = m_mapData.getMarkersList();
    RoomSaver saver(m_mapData, roomList);
    for (uint i = 0; i < m_mapData.getRoomsCount(); ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }

    uint roomsCount = saver.getRoomsCount();
    auto marksCount = static_cast<uint>(markerList.size());

    auto &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(roomsCount * 2 + marksCount);

    BaseMapSaveFilter filter;
    if (baseMapOnly) {
        filter.setMapData(&m_mapData);
        progressCounter.increaseTotalStepsBy(filter.prepareCount());
        filter.prepare(progressCounter);
    }

    JsonWorld world;
    world.addRooms(roomList, filter, progressCounter, baseMapOnly);

    QDir saveDir(m_fileName);
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

        world.writeMetadata(QFileInfo(destDir, "arda.json"), m_mapData);
        world.writeRoomIndex(roomIndexDir);
        world.writeZones(zoneDir, filter, progressCounter, baseMapOnly);
    } catch (const std::exception &e) {
        log(e.what());
        return false;
    }

    log("Writing data finished.");

    m_mapData.unsetDataChanged();
    emit sig_onDataSaved();

    return true;
}
