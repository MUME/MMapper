// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "PandoraMapStorage.h"

#include "../global/parserutils.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFlags.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../mapdata/mapdata.h"

#include <cassert>
#include <map>
#include <optional>
#include <vector>

#include <QRegularExpression>
#include <QXmlStreamReader>

struct NODISCARD ExitToDeathTrap final
{
    ExternalRoomId from;
    ExitDirEnum dir = ExitDirEnum::NONE;
};

struct NODISCARD PandoraMapStorage::LoadRoomHelper final
{
private:
    std::vector<ExitToDeathTrap> &m_exitsToDeathTrap;

public:
    explicit LoadRoomHelper(std::vector<ExitToDeathTrap> &exitsToDeathTrap)
        : m_exitsToDeathTrap{exitsToDeathTrap}
    {}

public:
    void addExitToDeathtrap(const ExternalRoomId from, const ExitDirEnum dirEnum)
    {
        m_exitsToDeathTrap.emplace_back(ExitToDeathTrap{from, dirEnum});
    }
};

PandoraMapStorage::PandoraMapStorage(const AbstractMapStorage::Data &data, QObject *parent)
    : AbstractMapStorage{data, parent}
{}

PandoraMapStorage::~PandoraMapStorage() = default;

NODISCARD static RoomTerrainEnum toTerrainType(const QString &str)
{
#define CASE2(UPPER, s) \
    do { \
        if (str == s) { \
            return RoomTerrainEnum::UPPER; \
        } \
    } while (false)
    CASE2(UNDEFINED, "undefined");
    CASE2(INDOORS, "indoors");
    CASE2(CITY, "city");
    CASE2(FIELD, "field");
    CASE2(FOREST, "forest");
    CASE2(HILLS, "hills");
    CASE2(MOUNTAINS, "mountains");
    CASE2(SHALLOW, "shallowwater");
    CASE2(WATER, "water");
    CASE2(RAPIDS, "rapids");
    CASE2(UNDERWATER, "underwater");
    CASE2(ROAD, "road");
    CASE2(BRUSH, "brush");
    CASE2(TUNNEL, "tunnel");
    CASE2(CAVERN, "cavern");
#undef CASE2
    return RoomTerrainEnum::UNDEFINED;
}

ExternalRawRoom PandoraMapStorage::loadRoom(QXmlStreamReader &xml, LoadRoomHelper &helper)
{
    ExternalRawRoom room;
    if ((false)) {
        // Not necessary
        room.setContents(RoomContents{});
    }

    room.status = RoomStatusEnum::Permanent;
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "room")) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == "room") {
                room.setId(
                    ExternalRoomId{static_cast<uint32_t>(xml.attributes().value("id").toInt())});

                // Terrain
                const auto terrainString = xml.attributes().value("terrain").toString().toLower();
                room.setTerrainType(toTerrainType(terrainString));

                // Coordinate
                const auto x = xml.attributes().value("x").toInt();
                const auto y = xml.attributes().value("y").toInt();
                const auto z = xml.attributes().value("z").toInt();
                room.setPosition(Coordinate{x, y, z});

            } else if (xml.name() == "roomname") {
                room.setName(mmqt::makeRoomName(xml.readElementText()));
            } else if (xml.name() == "desc") {
                room.setDescription(mmqt::makeRoomDesc(xml.readElementText().replace("|", "\n")));
            } else if (xml.name() == "note") {
                room.setNote(mmqt::makeRoomNote(xml.readElementText()));
            } else if (xml.name() == "exits") {
                loadExits(room, xml, helper);
            }
        }
        xml.readNext();
    }
    return room;
}

void PandoraMapStorage::loadExits(ExternalRawRoom &room,
                                  QXmlStreamReader &xml,
                                  LoadRoomHelper &helper)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "exits")) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == "exit") {
                const auto attr = xml.attributes();
                if (attr.hasAttribute("dir") && attr.hasAttribute("to")
                    && attr.hasAttribute("door")) {
                    const auto dirStr = xml.attributes().value("dir").toString();
                    const auto dir = Mmapper2Exit::dirForChar(dirStr.at(0).toLatin1());

                    ExternalRawExit &exit = room.exits[dir];
                    // REVISIT: This is now controlled by the map
                    if (true) {
                        exit.setExitFlags(exit.getExitFlags() | ExitFlagEnum::EXIT);
                    }

                    const auto to = xml.attributes().value("to").toString();
                    if (to == "DEATH") {
                        helper.addExitToDeathtrap(room.id, dir);
                    } else if (to == "UNDEFINED") {
                        exit.setExitFlags(exit.getExitFlags() | ExitFlagEnum::UNMAPPED);
                    } else {
                        bool ok = false;
                        const int id = to.toInt(&ok);
                        if (!ok || id < 0) {
                            throw std::runtime_error("invalid room");
                        }

                        exit.outgoing.insert(ExternalRoomId{static_cast<uint32_t>(id)});
                    }

                    const auto doorName = xml.attributes().value("door").toString();
                    if (doorName != nullptr && !doorName.isEmpty()) {
                        // REVISIT: This is now controlled by the map
                        if (true) {
                            exit.setExitFlags(exit.getExitFlags() | ExitFlagEnum::DOOR);
                        }
                        if (doorName != "exit") {
                            // REVISIT: why do we assume it's hidden?
                            // Does the map format only store hidden door names?
                            exit.setDoorFlags(DoorFlags{DoorFlagEnum::HIDDEN});
                            exit.setDoorName(mmqt::makeDoorName(doorName));
                        }
                    }
                } else {
                    qDebug() << "Room" << room.getId().asUint32() << "was missing exit attributes";
                }
            }
        }
        xml.readNext();
    }
}

std::optional<RawMapLoadData> PandoraMapStorage::virt_loadData()
{
    log("Loading data ...");

    RawMapLoadData result;
    result.filename = getFilename();
    result.readonly = true;

    auto &progressCounter = getProgressCounter();
    progressCounter.reset();

    QFile *const file = getFile();
    QXmlStreamReader xml{file};

    // Discover total number of rooms
    const QString &file_fileName = file->fileName();
    if (xml.readNextStartElement() && xml.error() != QXmlStreamReader::NoError) {
        qWarning() << "File cannot be read" << file_fileName;
        return std::nullopt;
    } else if (xml.name() != "map") {
        qWarning() << "File does not start with element 'map'" << file_fileName;
        return std::nullopt;
    } else if (xml.attributes().isEmpty() || !xml.attributes().hasAttribute("rooms")) {
        qWarning() << "'map' element did not have a 'rooms' attribute" << file_fileName;
        return std::nullopt;
    }

    const uint32_t roomsCount = static_cast<uint32_t>(xml.attributes().value("rooms").toInt());
    progressCounter.increaseTotalStepsBy(roomsCount);
    log(QString("Expected number of rooms: %1").arg(roomsCount));

    std::vector<ExternalRawRoom> loading;
    loading.reserve(roomsCount);

    std::vector<ExitToDeathTrap> exitsToDeathTrap;
    LoadRoomHelper helper{exitsToDeathTrap};

    progressCounter.setCurrentTask(ProgressMsg{"reading rooms"});
    for (uint32_t i = 0; i < roomsCount; ++i) {
        while (xml.readNextStartElement() && !xml.hasError()) {
            if (xml.name() == "room") {
                loading.emplace_back(loadRoom(xml, helper));
                progressCounter.step();
            }
        }
    }

    log(QString("Finished reading %1 rooms.").arg(loading.size()));
    file->close();

    if (!exitsToDeathTrap.empty()) {
        log(QString("Adding %1 death trap rooms").arg(exitsToDeathTrap.size()));

        loading.reserve(loading.size() + exitsToDeathTrap.size());

        struct NODISCARD Map final : std::map<ExternalRoomId, uint32_t>
        {
            NODISCARD bool contains(const ExternalRoomId id) const { return find(id) != end(); }
        };

        Map index;
        ExternalRoomId max{0};
        for (uint32_t i = 0; i < roomsCount; ++i) {
            const auto xid = loading[i].getId();
            if (xid == INVALID_EXTERNAL_ROOMID) {
                throw std::runtime_error("invalid room ID detected");
            }

            if (xid > max) {
                max = xid;
            }

            if (index.contains(xid)) {
                // This isn't our responsibility to enforce, but...
                throw std::runtime_error("duplicate room ID detected");
            }

            index[xid] = i;
        }

        // note: mutable lambda is required for "next" to be writable.
        auto allocId = [next = max.next()]() mutable -> ExternalRoomId {
            auto result_id = next;
            next = next.next();
            return result_id;
        };

        auto newDeathTrapRoom = [](const ExternalRoomId id,
                                   const Coordinate &pos) -> ExternalRawRoom {
            ExternalRawRoom result_room;
            result_room.setId(id);
            result_room.setPosition(pos);
            result_room.setTerrainType(RoomTerrainEnum::INDOORS);
            result_room.setLoadFlags(RoomLoadFlags{RoomLoadFlagEnum::DEATHTRAP});
            return result_room;
        };

        for (const auto &x : exitsToDeathTrap) {
            ExternalRawRoom &from = loading.at(index.at(x.from));
            // REVISIT: Should this be 2 * exitDir for NESW?
            const Coordinate pos = from.getPosition() + exitDir(x.dir);
            const auto id = allocId();
            from.getExit(x.dir).outgoing.insert(id);
            loading.emplace_back(newDeathTrapRoom(id, pos));
        }
    }

    result.rooms = std::move(loading);
    log("Finished loading.");

    return result;
}

void PandoraMapStorage::log(const QString &msg)
{
    emit sig_log("PandoraMapStorage", msg);
    qInfo() << msg;
}
