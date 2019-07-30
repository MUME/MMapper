// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "PandoraMapStorage.h"

#include <cassert>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomfactory.h"
#include "../parser/parserutils.h"
#include "mapstorage.h"

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
    //clear previous map
    m_mapData.clear();
    try {
        return mergeData();
    } catch (const std::exception &ex) {
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        emit log("PandoraMapStorage", msg);
        qWarning().noquote() << msg;
        m_mapData.clear();
        return false;
    }
}

static RoomTerrainEnum toTerrainType(const QString &str)
{
#define CASE2(UPPER, s) \
    do { \
        if (str == s) \
            return RoomTerrainEnum::UPPER; \
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
#undef CASE3
    return RoomTerrainEnum::UNDEFINED;
}

Room *PandoraMapStorage::loadRoom(QXmlStreamReader &xml)
{
    Room *room = factory.createRoom();
    room->setPermanent();
    room->setDynamicDescription("");
    room->setUpToDate();
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "room")) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == "room") {
                room->setId(RoomId{static_cast<uint32_t>(xml.attributes().value("id").toInt())});

                // Terrain
                const auto terrainString = xml.attributes().value("terrain").toString().toLower();
                room->setTerrainType(toTerrainType(terrainString));

                // Coordinate
                const auto x = xml.attributes().value("x").toInt();
                const auto y = xml.attributes().value("y").toInt() * -1;
                const auto z = xml.attributes().value("z").toInt();
                room->setPosition(Coordinate{x, y, z} + basePosition);

            } else if (xml.name() == "roomname") {
                room->setName(xml.readElementText());
            } else if (xml.name() == "desc") {
                room->setStaticDescription(xml.readElementText().replace("|", "\n"));
            } else if (xml.name() == "note") {
                room->setNote(xml.readElementText());
            } else if (xml.name() == "exits") {
                loadExits(*room, xml);
            }
        }
        xml.readNext();
    }
    return room;
}

void PandoraMapStorage::loadExits(Room &room, QXmlStreamReader &xml)
{
    ExitsList &eList = room.getExitsList();
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "exits")) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == "exit") {
                const auto attr = xml.attributes();
                if (attr.hasAttribute("dir") && attr.hasAttribute("to")
                    && attr.hasAttribute("door")) {
                    const auto dirStr = xml.attributes().value("dir").toString();
                    const auto dir = Mmapper2Exit::dirForChar(dirStr.at(0).toLatin1());
                    Exit &exit = eList[dir];
                    exit.updateExit(ExitFlags{ExitFlagEnum::EXIT});

                    const auto to = xml.attributes().value("to").toString();
                    if (to == "DEATH") {
                        // REVISIT: Create a room for the death trap?
                    } else if (to != "UNDEFINED") {
                        exit.addOut(RoomId{static_cast<uint32_t>(to.toInt())});
                    }

                    const auto doorName = xml.attributes().value("door").toString();
                    if (doorName != nullptr && !doorName.isEmpty()) {
                        exit.updateExit(ExitFlags{ExitFlagEnum::DOOR});
                        if (doorName != "exit") {
                            exit.setDoorFlags(DoorFlags{DoorFlagEnum::HIDDEN});
                            exit.setDoorName(doorName);
                        }
                    }
                } else {
                    qDebug() << "Room" << room.getId().asUint32() << "was missing attributes";
                }
            }
        }
        xml.readNext();
    }
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

        emit log("PandoraMapStorage", "Loading data ...");

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
        emit log("PandoraMapStorage", QString("Number of rooms: %1").arg(roomsCount));

        for (uint32_t i = 0; i < roomsCount; i++) {
            while (xml.readNextStartElement() && !xml.hasError()) {
                if (xml.name() == "room") {
                    Room *room = loadRoom(xml);

                    progressCounter.step();
                    m_mapData.insertPredefinedRoom(*room);
                }
            }
        }

        // Set base position
        m_mapData.setPosition(Coordinate{});

        emit log("PandoraMapStorage", "Finished loading.");
        m_file->close();

        if (m_mapData.getRoomsCount() == 0u) {
            return false;
        }

        m_mapData.setFileName(m_fileName, true);
        m_mapData.unsetDataChanged();
    }

    m_mapData.checkSize();
    emit onDataLoaded();
    return true;
}

bool PandoraMapStorage::saveData(bool /* baseMapOnly */)
{
    return false;
}
