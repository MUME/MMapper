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
#include "mapstorage.h"

#include <cassert>

#include <QLatin1String>
#include <QRegularExpression>
#include <QXmlStreamReader>

PandoraMapStorage::PandoraMapStorage(MapData &mapdata,
                                     const QString &filename,
                                     QFile *const file,
                                     QObject *parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
{}

PandoraMapStorage::~PandoraMapStorage() = default;

void PandoraMapStorage::newData()
{
    qWarning() << "PandoraMapStorage does not implement newData()";
}

bool PandoraMapStorage::loadData()
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
    CASE2(DEATHTRAP, "deathtrap"); // Not supported by Pandora
#undef CASE2
    return RoomTerrainEnum::UNDEFINED;
}

SharedRoom PandoraMapStorage::loadRoom(QXmlStreamReader &xml)
{
    SharedRoom room = Room::createPermanentRoom(m_mapData);
    room->setContents(RoomContents{});
    room->setUpToDate();
    while (
        !(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == QLatin1String("room"))) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == QLatin1String("room")) {
                room->setId(RoomId{static_cast<uint32_t>(xml.attributes().value("id").toInt())});

                // Terrain
                const auto terrainString = xml.attributes().value("terrain").toString().toLower();
                room->setTerrainType(toTerrainType(terrainString));

                // Coordinate
                const auto x = xml.attributes().value("x").toInt();
                const auto y = xml.attributes().value("y").toInt();
                const auto z = xml.attributes().value("z").toInt();
                room->setPosition(Coordinate{x, y, z} + basePosition);

            } else if (xml.name() == QLatin1String("roomname")) {
                room->setName(RoomName{xml.readElementText()});
            } else if (xml.name() == QLatin1String("desc")) {
                room->setDescription(RoomDesc{xml.readElementText().replace("|", "\n")});
            } else if (xml.name() == QLatin1String("note")) {
                room->setNote(RoomNote{xml.readElementText()});
            } else if (xml.name() == QLatin1String("exits")) {
                loadExits(*room, xml);
            }
        }
        xml.readNext();
    }
    return room;
}

void PandoraMapStorage::loadExits(Room &room, QXmlStreamReader &xml)
{
    ExitsList copiedExits = room.getExitsList();
    while (!(xml.tokenType() == QXmlStreamReader::EndElement
             && xml.name() == QLatin1String("exits"))) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == QLatin1String("exit")) {
                const auto attr = xml.attributes();
                if (attr.hasAttribute("dir") && attr.hasAttribute("to")
                    && attr.hasAttribute("door")) {
                    const auto dirStr = xml.attributes().value("dir").toString();
                    const auto dir = Mmapper2Exit::dirForChar(dirStr.at(0).toLatin1());
                    Exit &exit = copiedExits[dir];
                    exit.setExitFlags(exit.getExitFlags() | ExitFlagEnum::EXIT);

                    const auto to = xml.attributes().value("to").toString();
                    if (to == "DEATH") {
                        // REVISIT: Create a room for the death trap?
                    } else if (to != "UNDEFINED") {
                        exit.addOut(RoomId{static_cast<uint32_t>(to.toInt())});
                    }

                    const auto doorName = xml.attributes().value("door").toString();
                    if (doorName != nullptr && !doorName.isEmpty()) {
                        exit.setExitFlags(exit.getExitFlags() | ExitFlagEnum::DOOR);
                        if (doorName != "exit") {
                            exit.setDoorFlags(DoorFlags{DoorFlagEnum::HIDDEN});
                            exit.setDoorName(DoorName{doorName});
                        }
                    }
                } else {
                    qDebug() << "Room" << room.getId().asUint32() << "was missing attributes";
                }
            }
        }
        xml.readNext();
    }
    room.setExitsList(copiedExits);
}

bool PandoraMapStorage::mergeData()
{
    {
        MapFrontendBlocker blocker(m_mapData);

        basePosition = m_mapData.getMax();
        if (basePosition.x + basePosition.y + basePosition.z != 0) {
            basePosition.y = 0;
            basePosition.x = 0;
            basePosition.z = -1;
        }

        log("Loading data ...");

        auto &progressCounter = getProgressCounter();
        progressCounter.reset();

        QXmlStreamReader xml(m_file);
        m_mapData.setDataChanged();

        // Discover total number of rooms
        if (xml.readNextStartElement() && xml.error() != QXmlStreamReader::NoError) {
            qWarning() << "File cannot be read" << m_file->fileName();
            return false;
        } else if (xml.name() != QLatin1String("map")) {
            qWarning() << "File does not start with element 'map'" << m_file->fileName();
            return false;
        } else if (xml.attributes().isEmpty() || !xml.attributes().hasAttribute("rooms")) {
            qWarning() << "'map' element did not have a 'rooms' attribute" << m_file->fileName();
            return false;
        }
        const uint32_t roomsCount = static_cast<uint32_t>(xml.attributes().value("rooms").toInt());
        progressCounter.increaseTotalStepsBy(roomsCount);
        log(QString("Number of rooms: %1").arg(roomsCount));

        for (uint32_t i = 0; i < roomsCount; i++) {
            while (xml.readNextStartElement() && !xml.hasError()) {
                if (xml.name() == QLatin1String("room")) {
                    SharedRoom room = loadRoom(xml);
                    progressCounter.step();
                    m_mapData.insertPredefinedRoom(room);
                }
            }
        }

        // Set base position
        m_mapData.setPosition(Coordinate{});

        log("Finished loading.");
        m_file->close();

        if (m_mapData.getRoomsCount() == 0u) {
            return false;
        }

        m_mapData.setFileName(m_fileName, true);
        m_mapData.unsetDataChanged();
    }

    m_mapData.checkSize();
    emit sig_onDataLoaded();
    return true;
}

bool PandoraMapStorage::saveData(bool /* baseMapOnly */)
{
    return false;
}
