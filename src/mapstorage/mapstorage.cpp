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
#include "basemapsavefilter.h"
#include "configuration.h"
#include "infomark.h"
#include "mapdata.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "oldconnection.h"
#include "olddoor.h"
#include "oldroom.h"
#include "patterns.h"
#include "progresscounter.h"
#include "roomsaver.h"
#ifndef MMAPPER_NO_QTIOCOMPRESSOR
#include "qtiocompressor.h"
#endif

#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QMessageBox>

#include <cassert>
#include <iostream>

#define MINUMUM_STATIC_LINES 1
#define CURRENT_SCHEMA_VERSION 042

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

Room *MapStorage::loadOldRoom(QDataStream &stream, ConnectionList &connectionList)
{
    Room *room = factory.createRoom();
    room->setPermanent();

    // set default values
    RoomTerrainType terrainType = RTT_UNDEFINED;
    RoomPortableType portableType = RPT_UNDEFINED;
    RoomRidableType ridableType = RRT_UNDEFINED;
    RoomSundeathType sundeathType = RST_UNDEFINED;
    RoomLightType lightType = RLT_UNDEFINED;
    RoomAlignType alignType = RAT_UNDEFINED;
    RoomMobFlags mobFlags = 0;
    RoomLoadFlags loadFlags = 0;
    QString vqstr;
    quint16 vquint16;
    quint32 vquint32;
    qint8 vqint8;

    stream >> vqstr;
    room->replace(R_NAME, vqstr);

    stream >> vqstr;
    QStringList list = vqstr.split('\n');
    QStringList::iterator i;
    bool readingStaticDescLines = true;
    QString staticRoomDesc;
    QString dynamicRoomDesc;

    qint32 lineCount = 0;

    for (i = list.begin(); i != list.end(); ++i) {
        if ((*i) != "") {
            if ((lineCount >= MINUMUM_STATIC_LINES)
                && ((!readingStaticDescLines) || Patterns::matchDynamicDescriptionPatterns(*i))) {
                readingStaticDescLines = false;
                dynamicRoomDesc += (*i) + "\n";
                lineCount++;
            } else {
                staticRoomDesc += (*i) + "\n";
                lineCount++;
            }
        }
    }

    room->replace(R_DESC, staticRoomDesc);
    room->replace(R_DYNAMICDESC, dynamicRoomDesc);

    stream >> vquint16; //roomTerrain
    terrainType = static_cast<RoomTerrainType>(vquint16 + 1);

    stream >> vquint16; //roomMob
    switch (static_cast<int>(vquint16)) {
    case 1:
        SET(mobFlags, RMF_ANY);
        break; //PEACEFULL
    case 2:
        SET(mobFlags, RMF_SMOB);
        break; //AGGRESIVE
    case 3:
        SET(mobFlags, RMF_QUEST);
        break; //QUEST
    case 4:
        SET(mobFlags, RMF_SHOP);
        break; //SHOP
    case 5:
        SET(mobFlags, RMF_RENT);
        break; //RENT
    case 6:
        SET(mobFlags, RMF_GUILD);
        break; //GUILD
    default:
        break;
    }

    stream >> vquint16; //roomLoad
    switch (static_cast<int>(vquint16)) {
    case 1:
        SET(loadFlags, RLF_TREASURE);
        break; //TREASURE
    case 2:
        SET(loadFlags, RLF_HERB);
        break; //HERB
    case 3:
        SET(loadFlags, RLF_KEY);
        break; //KEY
    case 4:
        SET(loadFlags, RLF_WATER);
        break; //WATER
    case 5:
        SET(loadFlags, RLF_FOOD);
        break; //FOOD
    case 6:
        SET(loadFlags, RLF_HORSE);
        break; //HORSE
    case 7:
        SET(loadFlags, RLF_WARG);
        break; //WARG
    case 8:
        SET(loadFlags, RLF_TOWER);
        break; //TOWER
    case 9:
        SET(loadFlags, RLF_ATTENTION);
        break; //ATTENTION
    case 10:
        SET(loadFlags, RLF_BOAT);
        break; //BOAT
    default:
        break;
    }

    stream >> vquint16; //roomLocation { INDOOR, OUTSIDE }

    stream >> vquint16; //roomPortable { PORT, NOPORT }
    portableType = static_cast<RoomPortableType>(vquint16 + 1);
    stream >> vquint16; //roomLight { DARK, LIT }
    lightType = static_cast<RoomLightType>(vquint16 + 1);
    stream >> vquint16; //roomAlign { GOOD, NEUTRAL, EVIL }
    alignType = static_cast<RoomAlignType>(vquint16 + 1);

    stream >> vquint32; //roomFlags

    if (ISSET(vquint32, bit2)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_NORTH), EF_EXIT);
    }
    if (ISSET(vquint32, bit3)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_SOUTH), EF_EXIT);
    }
    if (ISSET(vquint32, bit4)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_EAST), EF_EXIT);
    }
    if (ISSET(vquint32, bit5)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_WEST), EF_EXIT);
    }
    if (ISSET(vquint32, bit6)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_UP), EF_EXIT);
    }
    if (ISSET(vquint32, bit7)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_DOWN), EF_EXIT);
    }
    if (ISSET(vquint32, bit8)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_NORTH), EF_DOOR);
        Mmapper2Exit::orExitFlags(room->exit(ED_NORTH), EF_NO_MATCH);
    }
    if (ISSET(vquint32, bit9)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_SOUTH), EF_DOOR);
        Mmapper2Exit::orExitFlags(room->exit(ED_SOUTH), EF_NO_MATCH);
    }
    if (ISSET(vquint32, bit10)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_EAST), EF_DOOR);
        Mmapper2Exit::orExitFlags(room->exit(ED_EAST), EF_NO_MATCH);
    }
    if (ISSET(vquint32, bit11)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_WEST), EF_DOOR);
        Mmapper2Exit::orExitFlags(room->exit(ED_WEST), EF_NO_MATCH);
    }
    if (ISSET(vquint32, bit12)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_UP), EF_DOOR);
        Mmapper2Exit::orExitFlags(room->exit(ED_UP), EF_NO_MATCH);
    }
    if (ISSET(vquint32, bit13)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_DOWN), EF_DOOR);
        Mmapper2Exit::orExitFlags(room->exit(ED_DOWN), EF_NO_MATCH);
    }
    if (ISSET(vquint32, bit14)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_NORTH), EF_ROAD);
    }
    if (ISSET(vquint32, bit15)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_SOUTH), EF_ROAD);
    }
    if (ISSET(vquint32, bit16)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_EAST), EF_ROAD);
    }
    if (ISSET(vquint32, bit17)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_WEST), EF_ROAD);
    }
    if (ISSET(vquint32, bit18)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_UP), EF_ROAD);
    }
    if (ISSET(vquint32, bit19)) {
        Mmapper2Exit::orExitFlags(room->exit(ED_DOWN), EF_ROAD);
    }

    stream >> vqint8; //roomUpdated

    stream >> vqint8; //roomCheckExits
    Coordinate pos;
    stream >> (quint32 &) pos.x;
    stream >> (quint32 &) pos.y;

    room->setPosition(pos + basePosition);

    Connection *c;

    stream >> vquint32;
    if (vquint32 != 0) {
        c = connectionList[vquint32 - 1];
        if (c->getRoom(0) != nullptr) {
            c->setRoom(room, 1);
        } else {
            c->setRoom(room, 0);
        }
        c->setDirection(CD_UP, room);
    }
    stream >> vquint32;
    if (vquint32 != 0) {
        c = connectionList[vquint32 - 1];
        if (c->getRoom(0) != nullptr) {
            c->setRoom(room, 1);
        } else {
            c->setRoom(room, 0);
        }
        c->setDirection(CD_DOWN, room);
    }
    stream >> vquint32;
    if (vquint32 != 0) {
        c = connectionList[vquint32 - 1];
        if (c->getRoom(0) != nullptr) {
            c->setRoom(room, 1);
        } else {
            c->setRoom(room, 0);
        }
        c->setDirection(CD_EAST, room);
    }
    stream >> vquint32;
    if (vquint32 != 0) {
        c = connectionList[vquint32 - 1];
        if (c->getRoom(0) != nullptr) {
            c->setRoom(room, 1);
        } else {
            c->setRoom(room, 0);
        }
        c->setDirection(CD_WEST, room);
    }
    stream >> vquint32;
    if (vquint32 != 0) {
        c = connectionList[vquint32 - 1];
        if (c->getRoom(0) != nullptr) {
            c->setRoom(room, 1);
        } else {
            c->setRoom(room, 0);
        }
        c->setDirection(CD_NORTH, room);
    }
    stream >> vquint32;
    if (vquint32 != 0) {
        c = connectionList[vquint32 - 1];
        if (c->getRoom(0) != nullptr) {
            c->setRoom(room, 1);
        } else {
            c->setRoom(room, 0);
        }
        c->setDirection(CD_SOUTH, room);
    }

    // store imported values
    room->replace(R_TERRAINTYPE, terrainType);
    room->replace(R_LIGHTTYPE, lightType);
    room->replace(R_ALIGNTYPE, alignType);
    room->replace(R_PORTABLETYPE, portableType);
    room->replace(R_RIDABLETYPE, ridableType);
    room->replace(R_SUNDEATHTYPE, sundeathType);
    room->replace(R_MOBFLAGS, mobFlags);
    room->replace(R_LOADFLAGS, loadFlags);

    return room;
}

Room *MapStorage::loadRoom(QDataStream &stream, qint32 version)
{
    QString vqba;
    quint32 vquint32;
    quint8 vquint8;
    quint16 vquint16;
    Room *room = factory.createRoom();
    room->setPermanent();

    stream >> vqba;
    room->replace(R_NAME, vqba);
    stream >> vqba;
    room->replace(R_DESC, vqba);
    stream >> vqba;
    room->replace(R_DYNAMICDESC, vqba);
    stream >> vquint32;
    room->setId(vquint32 + baseId);
    stream >> vqba;
    room->replace(R_NOTE, vqba);
    stream >> vquint8;
    room->replace(R_TERRAINTYPE, vquint8);
    stream >> vquint8;
    room->replace(R_LIGHTTYPE, vquint8);
    stream >> vquint8;
    room->replace(R_ALIGNTYPE, vquint8);
    stream >> vquint8;
    room->replace(R_PORTABLETYPE, vquint8);
    if (version >= 030) {
        stream >> vquint8;
    } else {
        vquint8 = 0;
    }
    room->replace(R_RIDABLETYPE, vquint8);
    if (version >= 041) {
        stream >> vquint8;
    } else {
        vquint8 = 0;
    }
    room->replace(R_SUNDEATHTYPE, vquint8);
    if (version >= 041) {
        stream >> vquint32;
        room->replace(R_MOBFLAGS, vquint32);
        stream >> vquint32;
        room->replace(R_LOADFLAGS, vquint32);
    } else {
        stream >> vquint16;
        room->replace(R_MOBFLAGS, vquint16);
        stream >> vquint16;
        room->replace(R_LOADFLAGS, vquint16);
    }

    stream >> vquint8; //roomUpdated
    if (vquint8 != 0u) {
        room->setUpToDate();
    }
    Coordinate c;
    stream >> const_cast<qint32 &>(c.x);
    stream >> const_cast<qint32 &>(c.y);
    stream >> const_cast<qint32 &>(c.z);

    room->setPosition(c + basePosition);
    loadExits(room, stream, version);
    return room;
}

void MapStorage::loadExits(Room *room, QDataStream &stream, qint32 version)
{
    quint8 vquint8; // To read generic 8-bit value

    ExitsList &eList = room->getExitsList();
    for (int i = 0; i < 7; ++i) {
        Exit &e = eList[i];

        // Read the exit flags
        ExitFlags flags;
        if (version >= 041) {
            // Exit flags are stored with 16 bits in version >= 041
            stream >> flags;
            e[E_FLAGS] = flags;
        } else {
            // Exit flags were stored with 8 bits in version < 041
            stream >> vquint8;
            if (ISSET(vquint8, EF_DOOR)) {
                SET(vquint8, EF_EXIT);
            }
            e[E_FLAGS] = vquint8;
        }

        DoorFlags dFlags;
        if (version >= 040) {
            // Door flags are stored with 16 bits in version >= 040
            stream >> dFlags;
            e[E_DOORFLAGS] = dFlags;
        } else {
            // Door flags were stored with 8 bits in version < 040
            stream >> vquint8;
            e[E_DOORFLAGS] = vquint8;
        }

        DoorName dName;
        stream >> dName;
        e[E_DOORNAME] = dName;

        quint32 connection;
        for (stream >> connection; connection != UINT_MAX; stream >> connection) {
            e.addIn(connection + baseId);
        }
        for (stream >> connection; connection != UINT_MAX; stream >> connection) {
            e.addOut(connection + baseId);
        }
    }
}

bool MapStorage::loadData()
{
    //clear previous map
    m_mapData.clear();
    return mergeData();
}

bool MapStorage::mergeData()
{
    {
        MapFrontendBlocker blocker(m_mapData);

        baseId = m_mapData.getMaxId() + 1;
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
        m_progressCounter->reset();

        QDataStream stream(m_file);

        Room *room = nullptr;
        InfoMark *mark = nullptr;

        quint32 magic;
        qint32 version;
        quint32 index = 0;
        quint32 roomsCount = 0;
        quint32 connectionsCount = 0;
        quint32 marksCount = 0;

        m_mapData.setDataChanged();

        // Read the version and magic
        stream >> magic;
        if (magic != 0xFFB2AF01) {
            return false;
        }
        stream >> version;
        if (version != 042 && version != 041 && version != 040 && version != 031 && version != 030
            && version != 020 && version != 021 && version != 007) {
            bool isNewer = version >= CURRENT_SCHEMA_VERSION;
            QMessageBox::critical(dynamic_cast<QWidget *>(parent()),
                                  "MapStorage Error",
                                  QString("This map has schema version %1 which is too %2.\r\n\r\n"
                                          "Please %3 MMapper.")
                                      .arg(version, 0, 8)
                                      .arg(isNewer ? "new" : "old")
                                      .arg(isNewer ? "upgrade to the latest"
                                                   : "try an older version of"));
            return false;
        }

        // Force serialization to Qt4.8 because Qt5 has broke backwards compatability with QDateTime serialization
        // http://doc.qt.io/qt-5/sourcebreaks.html#changes-to-qdate-qtime-and-qdatetime
        // http://doc.qt.io/qt-5/qdatastream.html#versioning
        stream.setVersion(QDataStream::Qt_4_8);

        QBuffer buffer;
        if (version >= 042) {
            QByteArray compressedData(stream.device()->readAll());
            QByteArray uncompressedData = qUncompress(compressedData);
            buffer.setData(uncompressedData);
            buffer.open(QIODevice::ReadOnly);
            stream.setDevice(&buffer);
            emit log("MapStorage", "Uncompressed data");

        } else if (version <= 041 && version >= 031) {
#ifndef MMAPPER_NO_QTIOCOMPRESSOR
            auto *compressor = new QtIOCompressor(m_file);
            compressor->open(QIODevice::ReadOnly);
            stream.setDevice(compressor);
#else
            QMessageBox::critical(
                (QWidget *) parent(),
                "MapStorage Error",
                "MMapper could not load this map because it is too old.\r\n\r\n"
                "Please recompile MMapper with QtIOCompressor support and try again.");
            return false;
#endif
        }
        emit log("MapStorage", QString("Schema version: %1").arg(version, 0, 8));

        stream >> roomsCount;
        if (version < 020) {
            stream >> connectionsCount;
        }
        stream >> marksCount;

        m_progressCounter->increaseTotalStepsBy(roomsCount + connectionsCount + marksCount);

        Coordinate pos;

        //read selected room x,y
        // TODO(nschimme): Delete support for the old version due to octal constant nonsense
        if (version < 020) { // OLD VERSIONS SUPPORT CODE
            stream >> (quint32 &) pos.x;
            stream >> (quint32 &) pos.y;
        } else {
            stream >> static_cast<qint32 &>(pos.x);
            stream >> static_cast<qint32 &>(pos.y);
            stream >> static_cast<qint32 &>(pos.z);
        }

        pos += basePosition;

        m_mapData.setPosition(pos);

        emit log("MapStorage", QString("Number of rooms: %1").arg(roomsCount));

        ConnectionList connectionList;
        // create all pointers to connections
        for (index = 0; index < connectionsCount; index++) {
            auto *connection = new Connection;
            connectionList.append(connection);
        }

        RoomVector roomList(roomsCount);
        for (uint i = 0; i < roomsCount; ++i) {
            if (version < 020) { // OLD VERSIONS SUPPORT CODE
                room = loadOldRoom(stream, connectionList);
                room->setId(i + baseId);
                roomList[i] = room;
            } else {
                room = loadRoom(stream, version);
            }

            m_progressCounter->step();
            m_mapData.insertPredefinedRoom(*room);
        }

        if (version < 020) {
            emit log("MapStorage", QString("Number of connections: %1").arg(connectionsCount));
            ConnectionListIterator c(connectionList);
            while (c.hasNext()) {
                Connection *connection = c.next();
                loadOldConnection(connection, stream, roomList);
                translateOldConnection(connection);
                delete connection;

                m_progressCounter->step();
            }
            connectionList.clear();
        }

        emit log("MapStorage", QString("Number of info items: %1").arg(marksCount));

        MarkerList &markerList = m_mapData.getMarkersList();
        // create all pointers to items
        for (index = 0; index < marksCount; index++) {
            mark = new InfoMark();
            loadMark(mark, stream, version);
            markerList.append(mark);

            m_progressCounter->step();
        }

        emit log("MapStorage", "Finished loading.");
        buffer.close();
        m_file->close();

#ifndef MMAPPER_NO_QTIOCOMPRESSOR
        if (version <= 041 && version >= 031) {
            auto *compressor = dynamic_cast<QtIOCompressor *>(stream.device());
            compressor->close();
            delete compressor;
        }
#endif

        if (m_mapData.getRoomsCount() < 1) {
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
    QString vqstr;
    quint16 vquint16;
    quint32 vquint32;
    qint32 vqint32;

    qint32 postfix = basePosition.x + basePosition.y + basePosition.z;

    if (version < 020) { // OLD VERSIONS SUPPORT CODE
        stream >> vqstr;
        if (postfix != 0 && postfix != 1) {
            vqstr += QString("_m%1").arg(postfix);
        }
        mark->setName(vqstr);
        stream >> vqstr;
        mark->setText(vqstr);
        stream >> vquint16;
        mark->setType(static_cast<InfoMarkType>(vquint16));

        Coordinate pos;
        stream >> vquint32;
        pos.x = static_cast<qint32>(vquint32);
        pos.x = pos.x * 100 / 48 - 40;
        stream >> vquint32;
        pos.y = static_cast<qint32>(vquint32);
        pos.y = pos.y * 100 / 48 - 55;
        //pos += basePosition;
        pos.x += basePosition.x * 100;
        pos.y += basePosition.y * 100;
        pos.z += basePosition.z;
        mark->setPosition1(pos);

        stream >> vquint32;
        pos.x = static_cast<qint32>(vquint32);
        pos.x = pos.x * 100 / 48 - 40;
        stream >> vquint32;
        pos.y = static_cast<qint32>(vquint32);
        pos.y = pos.y * 100 / 48 - 55;
        //pos += basePosition;
        pos.x += basePosition.x * 100;
        pos.y += basePosition.y * 100;
        pos.z += basePosition.z;
        mark->setPosition2(pos);

        mark->setRotationAngle(0.0);
    } else {
        QString vqba;
        QDateTime vdatetime;
        quint8 vquint8;

        stream >> vqba;
        if (postfix != 0 && postfix != 1) {
            vqba += QString("_m%1").arg(postfix).toLatin1();
        }
        mark->setName(vqba);
        stream >> vqba;
        mark->setText(vqba);
        stream >> vdatetime;
        mark->setTimeStamp(vdatetime);
        stream >> vquint8;
        mark->setType(static_cast<InfoMarkType>(vquint8));
        if (version >= 040) {
            stream >> vquint8;
            mark->setClass(static_cast<InfoMarkClass>(vquint8));
            stream >> vquint32;
            mark->setRotationAngle(vquint32 / 100);
        }

        Coordinate pos;
        stream >> vqint32;
        pos.x = vqint32 /*-40*/;
        stream >> vqint32;
        pos.y = vqint32 /*-55*/;
        stream >> vqint32;
        pos.z = vqint32;
        pos.x += basePosition.x * 100;
        pos.y += basePosition.y * 100;
        pos.z += basePosition.z;
        //pos += basePosition;
        mark->setPosition1(pos);

        stream >> vqint32;
        pos.x = vqint32 /*-40*/;
        stream >> vqint32;
        pos.y = vqint32 /*-55*/;
        stream >> vqint32;
        pos.z = vqint32;
        pos.x += basePosition.x * 100;
        pos.y += basePosition.y * 100;
        pos.z += basePosition.z;
        //pos += basePosition;
        mark->setPosition2(pos);
    }
}

void MapStorage::translateOldConnection(Connection *c)
{
    Room *left = c->getRoom(LEFT);
    Room *right = c->getRoom(RIGHT);
    ConnectionDirection leftDir = c->getDirection(LEFT);
    ConnectionDirection rightDir = c->getDirection(RIGHT);
    ConnectionFlags cFlags = c->getFlags();

    if (leftDir != CD_NONE) {
        Exit &e = left->exit(leftDir);
        e.addOut(right->getId());
        ExitFlags eFlags = Mmapper2Exit::getFlags(e);
        if ((cFlags & CF_DOOR) != 0) {
            eFlags |= EF_NO_MATCH;
            eFlags |= EF_DOOR;
            e[E_DOORNAME] = c->getDoor(LEFT)->getName();
            e[E_DOORFLAGS] = c->getDoor(LEFT)->getFlags();
        }
        if ((cFlags & CF_RANDOM) != 0) {
            eFlags |= EF_RANDOM;
        }
        if ((cFlags & CF_CLIMB) != 0) {
            eFlags |= EF_CLIMB;
        }
        if ((cFlags & CF_SPECIAL) != 0) {
            eFlags |= EF_SPECIAL;
        }
        eFlags |= EF_EXIT;
        e[E_FLAGS] = eFlags;

        Exit &eR = right->exit(opposite(leftDir));
        eR.addIn(left->getId());
    }
    if (rightDir != CD_NONE) {
        Exit &eL = left->exit(opposite(rightDir));
        eL.addIn(right->getId());

        Exit &e = right->exit(rightDir);
        e.addOut(left->getId());
        ExitFlags eFlags = Mmapper2Exit::getFlags(e);
        if ((cFlags & CF_DOOR) != 0) {
            eFlags |= EF_DOOR;
            eFlags |= EF_NO_MATCH;
            e[E_DOORNAME] = c->getDoor(RIGHT)->getName();
            e[E_DOORFLAGS] = c->getDoor(RIGHT)->getFlags();
        }
        if ((cFlags & CF_RANDOM) != 0) {
            eFlags |= EF_RANDOM;
        }
        if ((cFlags & CF_CLIMB) != 0) {
            eFlags |= EF_CLIMB;
        }
        if ((cFlags & CF_SPECIAL) != 0) {
            eFlags |= EF_SPECIAL;
        }
        eFlags |= EF_EXIT;
        e[E_FLAGS] = eFlags;
    }
}

void MapStorage::loadOldConnection(Connection *connection, QDataStream &stream, RoomVector &roomList)
{
    quint16 vquint16;
    QString vqstr;
    quint32 vquint32;

    Room *r1 = nullptr, *r2 = nullptr;

    ConnectionFlags cf = 0;

    connection->setNote("");

    stream >> vquint16;
    auto ct = static_cast<ConnectionType>(vquint16 & (bit1 + bit2));
    cf = static_cast<ConnectionFlags>(vquint16 >> 2);
    /*
    switch (vquint16)
    {
    case 0: ct = CT_NORMAL ; break;  // connection normal
    case 1: ct = CT_LOOP   ; break;  // loop
    case 2: ct = CT_ONEWAY ; break;  // oneway
    case 4:  SET( cf, CF_DOOR)   ; break; // door
    case 8:  SET( cf, CF_CLIMB)  ; break; // climb
    case 16: SET( cf, CF_SPECIAL); break; // special
    case 32: SET( cf, CF_RANDOM) ; break; // random
    }
    */
    // DOOR TYPE: DNORMAL, DHIDDEN, DNORMALNEEDKEY, DHIDDENNEEDKEY
    DoorFlags doorFlags1 = 0, doorFlags2 = 0;
    DoorName doorName1, doorName2;
    stream >> vquint16;
    switch (vquint16) {
    case 0:
        break; //Normal
    case 1:
        SET(doorFlags1, DF_HIDDEN); //Hidden
        break;
    case 2:
        SET(doorFlags1, DF_NEEDKEY); //Normal+Key
        break;
    case 3:
        SET(doorFlags1, DF_HIDDEN); //Hidden+Key
        SET(doorFlags1, DF_NEEDKEY);
        break;
    }
    stream >> vquint16;
    switch (vquint16) {
    case 0:
        break; //Normal
    case 1:
        SET(doorFlags2, DF_HIDDEN); //Hidden
        break;
    case 2:
        SET(doorFlags2, DF_NEEDKEY); //Normal+Key
        break;
    case 3:
        SET(doorFlags2, DF_HIDDEN); //Hidden+Key
        SET(doorFlags2, DF_NEEDKEY);
        break;
    }

    stream >> vqstr;
    doorName1 = vqstr;
    stream >> vqstr;
    doorName2 = vqstr;

    stream >> vquint32;
    if (vquint32 != 0 && ((vquint32 - 1) < static_cast<uint>(roomList.size()))) {
        r1 = roomList[vquint32 - 1];
        if (ISSET(cf, CF_DOOR)) {
            connection->setDoor(new Door(doorName1, doorFlags1), LEFT);
        }
    }

    stream >> vquint32;
    if (vquint32 != 0 && ((vquint32 - 1) < static_cast<uint>(roomList.size()))) {
        r2 = roomList[vquint32 - 1];
        if (ISSET(cf, CF_DOOR)) {
            connection->setDoor(new Door(doorName2, doorFlags2), RIGHT);
        }
    }

    assert(r1 != nullptr); // if these are not valid,
    assert(r2 != nullptr); // the indices were out of bounds

    Room *room = nullptr;
    if (connection->getRoom(0) == nullptr) {
        room = connection->getRoom(1);
        if (room != r1) {
            connection->setRoom(r1, 0);
        } else {
            connection->setRoom(r2, 0);
        }
    }
    if (connection->getRoom(1) == nullptr) {
        room = connection->getRoom(0);
        if (room != r1) {
            connection->setRoom(r1, 1);
        } else {
            connection->setRoom(r2, 1);
        }
    }

    assert(connection->getRoom(0) != nullptr);
    assert(connection->getRoom(1) != nullptr);

    if (ISSET(cf, CF_DOOR)) {
        assert(connection->getDoor(LEFT) != nullptr);
        assert(connection->getDoor(RIGHT) != nullptr);
    }

    if (connection->getDirection(RIGHT) == CD_UNKNOWN) {
        ct = CT_ONEWAY;
        connection->setDirection(CD_NONE, RIGHT);
    } else if (connection->getDirection(LEFT) == CD_UNKNOWN) {
        ct = CT_ONEWAY;
        connection->setDirection(connection->getDirection(RIGHT), LEFT);
        connection->setDirection(CD_NONE, LEFT);
        Room *temp = connection->getRoom(LEFT);
        connection->setRoom(connection->getRoom(RIGHT), LEFT);
        connection->setRoom(temp, RIGHT);
    }

    if (connection->getRoom(0) == connection->getRoom(1)) {
        ct = CT_LOOP;
    }

    connection->setType(ct);
    connection->setFlags(cf);
}

void MapStorage::saveRoom(const Room *room, QDataStream &stream)
{
    stream << Mmapper2Room::getName(room);
    stream << Mmapper2Room::getDescription(room);
    stream << Mmapper2Room::getDynamicDescription(room);
    stream << static_cast<quint32>(room->getId());
    stream << Mmapper2Room::getNote(room);
    stream << static_cast<quint8>(Mmapper2Room::getTerrainType(room));
    stream << static_cast<quint8>(Mmapper2Room::getLightType(room));
    stream << static_cast<quint8>(Mmapper2Room::getAlignType(room));
    stream << static_cast<quint8>(Mmapper2Room::getPortableType(room));
    stream << static_cast<quint8>(Mmapper2Room::getRidableType(room));
    stream << static_cast<quint8>(Mmapper2Room::getSundeathType(room));
    stream << static_cast<quint32>(Mmapper2Room::getMobFlags(room));
    stream << static_cast<quint32>(Mmapper2Room::getLoadFlags(room));

    stream << static_cast<quint8>(room->isUpToDate());

    const Coordinate &pos = room->getPosition();
    stream << static_cast<qint32>(pos.x);
    stream << static_cast<qint32>(pos.y);
    stream << static_cast<qint32>(pos.z);
    saveExits(room, stream);
}

void MapStorage::saveExits(const Room *room, QDataStream &stream)
{
    const ExitsList &exitList = room->getExitsList();
    ExitsListIterator el(exitList);
    while (el.hasNext()) {
        const Exit &e = el.next();
        stream << Mmapper2Exit::getFlags(e);
        stream << Mmapper2Exit::getDoorFlags(e);
        stream << Mmapper2Exit::getDoorName(e);
        for (auto idx : e.inRange()) {
            stream << static_cast<quint32>(idx);
        }
        stream << static_cast<quint32> UINT_MAX;
        for (auto idx : e.outRange()) {
            stream << static_cast<quint32>(idx);
        }
        stream << static_cast<quint32> UINT_MAX;
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
    RoomSaver saver(&m_mapData, roomList);
    for (uint i = 0; i < m_mapData.getRoomsCount(); ++i) {
        m_mapData.lookingForRooms(&saver, i);
    }

    uint roomsCount = saver.getRoomsCount();
    uint marksCount = markerList.size();

    m_progressCounter->reset();
    m_progressCounter->increaseTotalStepsBy(roomsCount + marksCount);

    BaseMapSaveFilter filter;
    if (baseMapOnly) {
        filter.setMapData(&m_mapData);
        m_progressCounter->increaseTotalStepsBy(filter.prepareCount());
        filter.prepare(m_progressCounter);
        roomsCount = filter.acceptedRoomsCount();
    }

    // Compression step
    m_progressCounter->increaseTotalStepsBy(1);

    // Write a header with a "magic number" and a version
    fileStream << static_cast<quint32>(0xFFB2AF01);
    fileStream << static_cast<qint32>(CURRENT_SCHEMA_VERSION);

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
        const Room *room = roomit.next();
        if (baseMapOnly) {
            BaseMapSaveFilter::Action action = filter.filter(room);
            if (!room->isTemporary() && action != BaseMapSaveFilter::REJECT) {
                if (action == BaseMapSaveFilter::ALTER) {
                    Room copy = filter.alteredRoom(room);
                    saveRoom(&copy, stream);
                } else { // action == PASS
                    saveRoom(room, stream);
                }
            }
        } else {
            saveRoom(room, stream);
        }

        m_progressCounter->step();
    }

    // save items
    MarkerListIterator markerit(markerList);
    while (markerit.hasNext()) {
        InfoMark *mark = markerit.next();
        saveMark(mark, stream);

        m_progressCounter->step();
    }

    buffer.close();

    QByteArray uncompressedData(buffer.data());
    QByteArray compressedData = qCompress(uncompressedData);
    m_progressCounter->step();
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
