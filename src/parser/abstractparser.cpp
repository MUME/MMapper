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

#include "abstractparser.h"
#include "mmapper2event.h"
#include "mmapper2room.h"
#include "mmapper2exit.h"
#include "configuration.h"
#include "roomselection.h"
#include "mapdata.h"
#include "mumeclock.h"
#include "mapaction.h"

#include <unistd.h>

#include <QDesktopServices>
#include <QUrl>

const QString AbstractParser::nullString;
const QString AbstractParser::emptyString("");
const QByteArray AbstractParser::emptyByteArray("");

AbstractParser::AbstractParser(MapData *md, MumeClock *mc, QObject *parent)
    : QObject(parent)
{
    m_readingRoomDesc = false;
    m_descriptionReady = false;
    m_mumeClock = mc;
    m_mapData = md;
    m_trollExitMapping = false;

    m_exitsFlags = 0;
    m_promptFlags = 0;

    search_rs = NULL;
}

AbstractParser::~AbstractParser()
{
    if (search_rs != NULL)
        m_mapData->unselect(search_rs);
}

void AbstractParser::reset()
{
    emit log("Parser", "Disabling troll exit mapping");
    m_trollExitMapping = false;
    queue.clear();
}

void AbstractParser::emptyQueue()
{
    queue.clear();
}

void AbstractParser::parsePrompt(QString &prompt)
{
    m_promptFlags = 0;
    quint8 index = 0;
    int sv;

    emit sendPromptLineEvent(m_stringBuffer.toLatin1());

    switch (sv = (int)((prompt[index]).toLatin1())) {
    case 42:
        index++;
        m_promptFlags = LIT_ROOM;
        break; // *  indoor/sun (direct and indirect)
    case 33:
        index++;
        break;                        // !  artifical light
    case 41:
        index++;
        m_promptFlags = LIT_ROOM;
        break; // )  moon (direct and indirect)
    case 111:
        index++;                              // o  darkness
        // TODO: Enable when we know it is night only
        //m_promptFlags=DARK_ROOM;
        break;
    default:
        ;
    }

    switch ( sv = (int)(prompt[index]).toLatin1() ) {
    case 91:
        SET(m_promptFlags, RTT_INDOORS);
        break;   // [  // indoors
    case 35:
        SET(m_promptFlags, RTT_CITY);
        break;      // #  // city
    case 46:
        SET(m_promptFlags, RTT_FIELD);
        break;     // .  // field
    case 102:
        SET(m_promptFlags, RTT_FOREST);
        break;   // f  // forest
    case 40:
        SET(m_promptFlags, RTT_HILLS);
        break;     // (  // hills
    case 60:
        SET(m_promptFlags, RTT_MOUNTAINS);
        break; // <  // mountains
    case 37:
        SET(m_promptFlags, RTT_SHALLOW);
        break;   // %  // shallow
    case 126:
        SET(m_promptFlags, RTT_WATER);
        break;     // ~  // water
    case 87:
        SET(m_promptFlags, RTT_RAPIDS);
        break;    // W  // rapids
    case 85:
        SET(m_promptFlags, RTT_UNDERWATER);
        break;// U  // underwater
    case 43:
        SET(m_promptFlags, RTT_ROAD);
        break;      // +  // road
    case 61:
        SET(m_promptFlags, RTT_TUNNEL);
        break;    // =  // tunnel
    case 79:
        SET(m_promptFlags, RTT_CAVERN);
        break;    // O  // cavern
    case 58:
        SET(m_promptFlags, RTT_BRUSH);
        break;     // :  // brush
    default:
        ;
    }
    SET(m_promptFlags, PROMPT_FLAGS_VALID);
}

void AbstractParser::parseExits(QString &str)
{
    m_connectedRoomFlags = 0;
    m_exitsFlags = 0;
    ExitsFlagsType closedDoorFlag = 0;
    bool doors = false;
    bool closed = false;
    bool road = false;
    bool climb = false;
    bool portal = false;
    bool directSun = false;
    DirectionType dir = UNKNOWN;

    if (str.length() > 5 && str.at(5).toLatin1() != ':') {
        // Ainur exits
        emit sendToUser(str.toLatin1() + "\r\n");
        return ;
    }
    int length = str.length();
    for (int i = 7; i < length; i++) {
        switch ((int)(str.at(i).toLatin1())) {
        case 40:
            doors = true;
            break;              // (  // open door
        case 91:
            doors = true;
            closed = true;
            break;              // [  // closed door
        case 35:
            doors = true;
            break;              // #  // broken door
        case 61:
            road = true;
            break;               // =  // road
        case 45:
            road = true;
            break;               // -  // trail
        case 47:
            climb = true;
            break;               // /  // upward climb
        case 92:
            climb = true;
            break;               // \  // downward climb
        case 123:
            portal = true;
            break;               // {  // portal
        case 42:
            directSun = true;
            break;               // *  // sunlit room (troll/orc only)
        case 94:
            directSun = true;    // ^  // outdoors room (troll only)
            if (!m_trollExitMapping)
                emit log("Parser", "Autoenabling troll exit mapping mode.");
            m_trollExitMapping = true;
            break;

        case 32: // empty space means reset for next exit
            doors = false;
            closed = false;
            road = false;
            climb = false;
            portal = false;
            directSun = false;
            dir = UNKNOWN;
            break;

        case 110:  // n
            if ( (i + 2) < length && (str.at(i + 2).toLatin1()) == 'r') { //north
                i += 5;
                dir = NORTH;
            } else {
                i += 4; //none
                dir = NONE;
            }
            break;

        case 115:  // s
            i += 5;
            dir = SOUTH;
            break;

        case 101:  // e
            i += 4;
            dir = EAST;
            break;

        case 119:  // w
            i += 4;
            dir = WEST;
            break;

        case 117:  // u
            i += 2;
            dir = UP;
            break;

        case 100:  // d
            i += 4;
            dir = DOWN;
            break;
        default:
            ;
        }
        if (dir < NONE) {
            SET(m_exitsFlags, (EF_EXIT << (dir * 4)));
            if (climb)
                SET(m_exitsFlags, (EF_CLIMB << (dir * 4)));
            if (doors) {
                SET(m_exitsFlags, (EF_DOOR << (dir * 4)));
                if (closed)
                    SET(closedDoorFlag, (EF_DOOR << (dir * 4)));
            }
            if (road)
                SET(m_exitsFlags, (EF_ROAD << (dir * 4)));
            if (directSun)
                SET(m_connectedRoomFlags, (DIRECT_SUN_ROOM << (dir * 2)));
        }
    }

    // If there is't a portal then we can trust the exits
    if (!portal) {
        SET(m_exitsFlags, EXITS_FLAGS_VALID);
        SET(m_connectedRoomFlags, CONNECTED_ROOM_FLAGS_VALID);

        // Orcs and trolls can detect exits with direct sunlight
        bool foundDirectSunlight = m_connectedRoomFlags & ANY_DIRECT_SUNLIGHT;
        if (foundDirectSunlight || m_trollExitMapping) {
            for (uint dir = 0; dir < 6; ++dir) {
                ExitsFlagsType eThisExit = m_exitsFlags >> (dir * 4);
                ExitsFlagsType eThisClosed = closedDoorFlag >> (dir * 4);
                ConnectedRoomFlagsType cOtherRoom = m_connectedRoomFlags >> (dir * 2);

                // Do not flag indirect sunlight if there was a closed door, no exit, or we saw direct sunlight
                if (!(eThisExit & EF_EXIT) || (eThisClosed & EF_DOOR) || (cOtherRoom & DIRECT_SUN_ROOM) ) continue;

                // Flag indirect sun
                SET(m_connectedRoomFlags, (INDIRECT_SUN_ROOM << (dir * 2)));
            }
        }
    }

    const RoomSelection *rs = m_mapData->select();
    const Room *room = m_mapData->getRoom(getPosition(), rs);
    QByteArray cn = enhanceExits(room);
    m_mapData->unselect(rs);

    emit sendToUser(str.toLatin1() + cn);
}

const Coordinate AbstractParser::getPosition()
{
    Coordinate c;
    CommandQueue tmpqueue;

    if (!queue.isEmpty())
        tmpqueue.enqueue(queue.head());

    QList<Coordinate> cl = m_mapData->getPath(tmpqueue);
    if (!cl.isEmpty())
        c = cl.at(cl.size() - 1);
    else
        c = m_mapData->getPosition();
    return c;
}

void AbstractParser::emulateExits()
{
    Coordinate c = getPosition();
    const RoomSelection *rs = m_mapData->select();
    const Room *r = m_mapData->getRoom(c, rs);

    sendRoomExitsInfoToUser(r);

    m_mapData->unselect(rs);
}

QByteArray AbstractParser::enhanceExits(const Room *sourceRoom)
{
    QByteArray cn = " -";
    bool enhancedExits = false;

    const RoomSelection *rs = m_mapData->select();
    uint sourceId = sourceRoom->getId();
    for (uint i = 0; i < 6; i++) {
        QByteArray ef = emptyByteArray;
        const Exit &e = sourceRoom->exit(i);
        if (!ISSET(Mmapper2Exit::getFlags(e), EF_EXIT)) continue;

        if (ISSET(Mmapper2Exit::getFlags(e), EF_NO_FLEE)) {
            ef += "noflee";
        }
        if (ISSET(Mmapper2Exit::getFlags(e), EF_RANDOM)) {
            if (!ef.isEmpty()) ef += ",";
            ef += "random";
        }

        // Exit modifiers
        if (e.containsOut(sourceId)) {
            if (!ef.isEmpty()) ef += ",";
            ef += "loop";
        } else if (e.outBegin() != e.outEnd()) {
            // Check target room for exit information
            uint targetId = *e.outBegin();
            const Room *targetRoom = m_mapData->getRoom(targetId, rs);

            int exitCount = 0;
            bool oneway = false;
            bool hasNoFlee = false;
            if (!targetRoom->exit(Mmapper2Exit::opposite(i)).containsOut(sourceId)) {
                oneway = true;
            }
            for (int j = 0; j < 6; ++j) {
                const Exit &targetExit = targetRoom->exit(j);
                if (ISSET(Mmapper2Exit::getFlags(targetExit), EF_EXIT)) {
                    exitCount++;
                } else {
                    continue;
                }
                if (targetExit.containsOut(sourceId)) {
                    // Technically rooms can point back in a different direction
                    oneway = false;
                }
                if (ISSET(Mmapper2Exit::getFlags(targetExit), EF_NO_FLEE)) {
                    hasNoFlee = true;
                }
            }
            if (oneway) {
                if (!ef.isEmpty()) ef += ",";
                ef += "oneway";
            }
            if (hasNoFlee && exitCount == 1) {
                if (!ef.isEmpty()) ef += ",";
                ef += "hasnoflee";
            }

            // Terrain type exit modifiers
            RoomTerrainType targetRoomTerrainType = Mmapper2Room::getTerrainType(targetRoom);
            if (targetRoomTerrainType == RTT_UNDERWATER) {
                if (!ef.isEmpty()) ef += ",";
                ef += "underwater";
            } else if (targetRoomTerrainType == RTT_DEATHTRAP) {
                // Override all previous flags
                ef = "deathtrap";
            }
        }

        QByteArray dn = Mmapper2Exit::getDoorName(e).toLatin1();
        if (!dn.isEmpty() || !ef.isEmpty()) {
            enhancedExits = true;
            cn += " ";
            cn += Mmapper2Exit::charForDir((ExitDirection)i);
            cn += ":";
            if (!dn.isEmpty()) {
                cn += dn;
            }
            if (!ef.isEmpty()) {
                cn += "(" + ef + ")";
            }
        }
    }
    m_mapData->unselect(rs);

    if (!enhancedExits) {
        cn = "\r\n";
    } else {
        cn += ".\r\n";

    }

    return cn;
}

void AbstractParser::parseNewUserInput(IncomingData &data)
{
    switch (data.type) {
    case TDT_DELAY:
    case TDT_PROMPT:
    case TDT_MENU_PROMPT:
    case TDT_LOGIN:
    case TDT_LOGIN_PASSWORD:
    case TDT_TELNET:
    case TDT_SPLIT:
    case TDT_UNKNOWN:
        emit sendToMud(data.line);
        break;
    case TDT_CRLF:
        m_stringBuffer = QString::fromLatin1(data.line.constData(), data.line.size());
        m_stringBuffer = m_stringBuffer.simplified();
        if (parseUserCommands(m_stringBuffer))
            emit sendToMud(data.line);
        break;
    case TDT_LFCR:
        m_stringBuffer = QString::fromLatin1(data.line.constData(), data.line.size());
        m_stringBuffer = m_stringBuffer.simplified();
        if (parseUserCommands(m_stringBuffer))
            emit sendToMud(data.line);
        break;
    case TDT_LF:
        m_stringBuffer = QString::fromLatin1(data.line.constData(), data.line.size());
        m_stringBuffer = m_stringBuffer.simplified();
        if ( parseUserCommands(m_stringBuffer))
            emit sendToMud(data.line);
        break;
    }
}

QString compressDirections(QString original)
{
    QString ans;
    int curnum = 0;
    QChar curval;
    for (auto c : original) {
        if (curnum == 0) {
            curnum = 1;
            curval = c;
        } else {
            if (curval == c)
                curnum++;
            else {
                if (curnum > 1)
                    ans.append(QString::number(curnum));
                ans.append(curval);
                curnum = 1;
                curval = c;
            }
        }
    }
    if (curnum > 1)
        ans.append(QString::number(curnum));
    ans.append(curval);
    return ans;
}


class ShortestPathEmitter : public ShortestPathRecipient
{
    AbstractParser &parser;
public:
    ShortestPathEmitter(AbstractParser &parser) : parser(parser) {}
    void receiveShortestPath(RoomAdmin *admin, QVector<SPNode> spnodes, int endpoint)
    {

        const SPNode *spnode = &spnodes[endpoint];
        QString ss = (*spnode->r)[0].toString();
        QByteArray s = ("Distance " + QString::number(spnode->dist) + ": "  + ss + "\r\n").toLatin1();
        emit parser.sendToUser(s);
        QString dirs;
        while (spnode->parent >= 0) {
            if (&spnodes[spnode->parent] == spnode) {
                emit parser.sendToUser(("ERROR: loop\r\n"));
                break;
            }
            dirs.append(Mmapper2Exit::charForDir(spnode->lastdir));
            spnode = &spnodes[spnode->parent];
        }
        std::reverse(dirs.begin(), dirs.end());
        emit parser.sendToUser(("dirs: " + compressDirections(dirs) + "\r\n").toLatin1());
    }
};

void AbstractParser::searchCommand(RoomFilter f)
{
    if (search_rs != NULL)
        m_mapData->unselect(search_rs);
    search_rs = m_mapData->select();
    m_mapData->genericSearch(search_rs, f);
    emit m_mapData->updateCanvas();
    emit sendToUser((QString::number(search_rs->size()) + " rooms found.\r\n").toLatin1());
    sendPromptToUser();
}

void AbstractParser::dirsCommand(RoomFilter f)
{
    ShortestPathEmitter sp_emitter(*this);

    Coordinate c = m_mapData->getPosition();
    const RoomSelection *rs = m_mapData->select(c);
    const Room *r = rs->values().front();

    m_mapData->shortestPathSearch(r, &sp_emitter, f, 10, 0);
    sendPromptToUser();

    m_mapData->unselect(rs);
}

void AbstractParser::markCurrentCommand()
{
    if (search_rs != NULL)
        m_mapData->unselect(search_rs);
    search_rs = m_mapData->select();
    Coordinate c = getPosition();
    m_mapData->select(c);
    emit m_mapData->updateCanvas();
}

bool AbstractParser::parseUserCommands(QString &command)
{
    QString str = command;

    DoorActionType daction = DAT_NONE;
    bool dooraction = false;

    if (str.contains("$$DOOR")) {
        if (str.contains("$$DOOR_N$$")) {
            genericDoorCommand(command, NORTH);
            return false;
        } else if (str.contains("$$DOOR_S$$")) {
            genericDoorCommand(command, SOUTH);
            return false;
        } else if (str.contains("$$DOOR_E$$")) {
            genericDoorCommand(command, EAST);
            return false;
        } else if (str.contains("$$DOOR_W$$")) {
            genericDoorCommand(command, WEST);
            return false;
        } else if (str.contains("$$DOOR_U$$")) {
            genericDoorCommand(command, UP);
            return false;
        } else if (str.contains("$$DOOR_D$$")) {
            genericDoorCommand(command, DOWN);
            return false;
        } else if (str.contains("$$DOOR$$"))  {
            genericDoorCommand(command, UNKNOWN);
            return false;
        }
    }

    if (str.startsWith('_')) {
        if (str.startsWith("_vote")) {
            QDesktopServices::openUrl(
                QUrl("http://www.mudconnect.com/cgi-bin/vote_rank.cgi?mud=MUME+-+Multi+Users+In+Middle+Earth"));
            emit sendToUser("--->Thank you kindly for voting!\r\n");
            sendPromptToUser();
            return false;
        } else if (str.startsWith("_gt")) { // grouptell
            if (!str.section(" ", 1, 1).toLatin1().isEmpty()) {
                QByteArray data = str.remove(0, 4).toLatin1(); // 3 is length of "_gt "
                //qDebug( "Sending a G-tell from local user: %s", (const char *) data);
                emit sendGroupTellEvent(data);
                sendPromptToUser();
            }
            return false;
        } else if (str.startsWith("_time")) {
            MumeMoment moment = m_mumeClock->getMumeMoment();
            QByteArray data = m_mumeClock->toMumeTime(moment).toLatin1() + "\r\n";
            MumeClockPrecision precision = m_mumeClock->getPrecision();
            if (precision > MUMECLOCK_DAY) {
                MumeTime time = moment.toTimeOfDay();
                data += "It is currently ";
                if (time == TIME_DAWN)
                    data += "\033[31mdawn\033[0m";
                else if (time == TIME_DUSK)
                    data += "\033[34mdusk\033[0m and will be night";
                else if (time == TIME_NIGHT)
                    data += "\033[34mnight\033[0m";
                else
                    data += "\033[33mday\033[0m";

                data += " for " + m_mumeClock->toCountdown(moment).toLatin1() + " more ticks.\r\n";
            }
            emit sendToUser(data + "\r\n");
            sendPromptToUser();
            return false;
        } else if (str.startsWith("_name")) {
            if (str.section(" ", 1, 1) == "n") nameDoorCommand(str.section(" ", 2, 2), NORTH);
            if (str.section(" ", 1, 1) == "s") nameDoorCommand(str.section(" ", 2, 2), SOUTH);
            if (str.section(" ", 1, 1) == "e") nameDoorCommand(str.section(" ", 2, 2), EAST);
            if (str.section(" ", 1, 1) == "w") nameDoorCommand(str.section(" ", 2, 2), WEST);
            if (str.section(" ", 1, 1) == "u") nameDoorCommand(str.section(" ", 2, 2), UP);
            if (str.section(" ", 1, 1) == "d") nameDoorCommand(str.section(" ", 2, 2), DOWN);
            if (str.section(" ", 1, 1) == "s") nameDoorCommand(str.section(" ", 2, 2), SOUTH);
            return false;
        } else if (str.startsWith("_door") && str != "_doorhelp") {
            if (!str.section(" ", 2, 2).toLatin1().isEmpty())
                emit sendToUser("--->Incorrect Command. Perhaps you meant _name?\r\n");
            else {
                if (str.section(" ", 1, 1) == "n") toggleExitFlagCommand(EF_DOOR, NORTH);
                if (str.section(" ", 1, 1) == "s") toggleExitFlagCommand(EF_DOOR, SOUTH);
                if (str.section(" ", 1, 1) == "e") toggleExitFlagCommand(EF_DOOR, EAST);
                if (str.section(" ", 1, 1) == "w") toggleExitFlagCommand(EF_DOOR, WEST);
                if (str.section(" ", 1, 1) == "u") toggleExitFlagCommand(EF_DOOR, UP);
                if (str.section(" ", 1, 1) == "d") toggleExitFlagCommand(EF_DOOR, DOWN);
            }
            return false;
        } else if (str.startsWith("_hidden")) {
            if (str.section(" ", 1, 1) == "n") toggleDoorFlagCommand(DF_HIDDEN, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleDoorFlagCommand(DF_HIDDEN, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleDoorFlagCommand(DF_HIDDEN, EAST);
            if (str.section(" ", 1, 1) == "w") toggleDoorFlagCommand(DF_HIDDEN, WEST);
            if (str.section(" ", 1, 1) == "u") toggleDoorFlagCommand(DF_HIDDEN, UP);
            if (str.section(" ", 1, 1) == "d") toggleDoorFlagCommand(DF_HIDDEN, DOWN);
            return false;
        } else  if (str.startsWith("_needkey")) {
            if (str.section(" ", 1, 1) == "n") toggleDoorFlagCommand(DF_NEEDKEY, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleDoorFlagCommand(DF_NEEDKEY, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleDoorFlagCommand(DF_NEEDKEY, EAST);
            if (str.section(" ", 1, 1) == "w") toggleDoorFlagCommand(DF_NEEDKEY, WEST);
            if (str.section(" ", 1, 1) == "u") toggleDoorFlagCommand(DF_NEEDKEY, UP);
            if (str.section(" ", 1, 1) == "d") toggleDoorFlagCommand(DF_NEEDKEY, DOWN);
            return false;
        } else if (str.startsWith("_noblock")) {
            if (str.section(" ", 1, 1) == "n") toggleDoorFlagCommand(DF_NOBLOCK, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleDoorFlagCommand(DF_NOBLOCK, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleDoorFlagCommand(DF_NOBLOCK, EAST);
            if (str.section(" ", 1, 1) == "w") toggleDoorFlagCommand(DF_NOBLOCK, WEST);
            if (str.section(" ", 1, 1) == "u") toggleDoorFlagCommand(DF_NOBLOCK, UP);
            if (str.section(" ", 1, 1) == "d") toggleDoorFlagCommand(DF_NOBLOCK, DOWN);
            return false;
        } else if (str.startsWith("_nobreak")) {
            if (str.section(" ", 1, 1) == "n") toggleDoorFlagCommand(DF_NOBREAK, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleDoorFlagCommand(DF_NOBREAK, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleDoorFlagCommand(DF_NOBREAK, EAST);
            if (str.section(" ", 1, 1) == "w") toggleDoorFlagCommand(DF_NOBREAK, WEST);
            if (str.section(" ", 1, 1) == "u") toggleDoorFlagCommand(DF_NOBREAK, UP);
            if (str.section(" ", 1, 1) == "d") toggleDoorFlagCommand(DF_NOBREAK, DOWN);
            return false;
        } else if (str.startsWith("_nopick")) {
            if (str.section(" ", 1, 1) == "n") toggleDoorFlagCommand(DF_NOPICK, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleDoorFlagCommand(DF_NOPICK, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleDoorFlagCommand(DF_NOPICK, EAST);
            if (str.section(" ", 1, 1) == "w") toggleDoorFlagCommand(DF_NOPICK, WEST);
            if (str.section(" ", 1, 1) == "u") toggleDoorFlagCommand(DF_NOPICK, UP);
            if (str.section(" ", 1, 1) == "d") toggleDoorFlagCommand(DF_NOPICK, DOWN);
            return false;
        } else if (str.startsWith("_delayed")) {
            if (str.section(" ", 1, 1) == "n") toggleDoorFlagCommand(DF_DELAYED, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleDoorFlagCommand(DF_DELAYED, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleDoorFlagCommand(DF_DELAYED, EAST);
            if (str.section(" ", 1, 1) == "w") toggleDoorFlagCommand(DF_DELAYED, WEST);
            if (str.section(" ", 1, 1) == "u") toggleDoorFlagCommand(DF_DELAYED, UP);
            if (str.section(" ", 1, 1) == "d") toggleDoorFlagCommand(DF_DELAYED, DOWN);
            return false;
        } else if (str.startsWith("_exit")) {
            if (str.section(" ", 1, 1) == "n") toggleExitFlagCommand(EF_EXIT, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleExitFlagCommand(EF_EXIT, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleExitFlagCommand(EF_EXIT, EAST);
            if (str.section(" ", 1, 1) == "w") toggleExitFlagCommand(EF_EXIT, WEST);
            if (str.section(" ", 1, 1) == "u") toggleExitFlagCommand(EF_EXIT, UP);
            if (str.section(" ", 1, 1) == "d") toggleExitFlagCommand(EF_EXIT, DOWN);
            return false;
        } else if (str.startsWith("_road")) {
            if (str.section(" ", 1, 1) == "n") toggleExitFlagCommand(EF_ROAD, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleExitFlagCommand(EF_ROAD, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleExitFlagCommand(EF_ROAD, EAST);
            if (str.section(" ", 1, 1) == "w") toggleExitFlagCommand(EF_ROAD, WEST);
            if (str.section(" ", 1, 1) == "u") toggleExitFlagCommand(EF_ROAD, UP);
            if (str.section(" ", 1, 1) == "d") toggleExitFlagCommand(EF_ROAD, DOWN);
            return false;
        } else if (str.startsWith("_climb")) {
            if (str.section(" ", 1, 1) == "n") toggleExitFlagCommand(EF_CLIMB, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleExitFlagCommand(EF_CLIMB, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleExitFlagCommand(EF_CLIMB, EAST);
            if (str.section(" ", 1, 1) == "w") toggleExitFlagCommand(EF_CLIMB, WEST);
            if (str.section(" ", 1, 1) == "u") toggleExitFlagCommand(EF_CLIMB, UP);
            if (str.section(" ", 1, 1) == "d") toggleExitFlagCommand(EF_CLIMB, DOWN);
            return false;
        } else if (str.startsWith("_random")) {
            if (str.section(" ", 1, 1) == "n") toggleExitFlagCommand(EF_RANDOM, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleExitFlagCommand(EF_RANDOM, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleExitFlagCommand(EF_RANDOM, EAST);
            if (str.section(" ", 1, 1) == "w") toggleExitFlagCommand(EF_RANDOM, WEST);
            if (str.section(" ", 1, 1) == "u") toggleExitFlagCommand(EF_RANDOM, UP);
            if (str.section(" ", 1, 1) == "d") toggleExitFlagCommand(EF_RANDOM, DOWN);
            return false;
        } else if (str.startsWith("_special")) {
            if (str.section(" ", 1, 1) == "n") toggleExitFlagCommand(EF_SPECIAL, NORTH);
            if (str.section(" ", 1, 1) == "s") toggleExitFlagCommand(EF_SPECIAL, SOUTH);
            if (str.section(" ", 1, 1) == "e") toggleExitFlagCommand(EF_SPECIAL, EAST);
            if (str.section(" ", 1, 1) == "w") toggleExitFlagCommand(EF_SPECIAL, WEST);
            if (str.section(" ", 1, 1) == "u") toggleExitFlagCommand(EF_SPECIAL, UP);
            if (str.section(" ", 1, 1) == "d") toggleExitFlagCommand(EF_SPECIAL, DOWN);
            return false;
        } else if (str.startsWith("_lit")) {
            setRoomFieldCommand(RLT_LIT, R_LIGHTTYPE);
            return false;
        } else if (str.startsWith("_dark")) {
            setRoomFieldCommand(RLT_DARK, R_LIGHTTYPE);
            return false;
        } else if (str.startsWith("_nosundeath")) {
            setRoomFieldCommand(RST_NOSUNDEATH, R_SUNDEATHTYPE);
            return false;
        } else if (str.startsWith("_sundeath")) {
            setRoomFieldCommand(RST_SUNDEATH, R_SUNDEATHTYPE);
            return false;
        } else if (str.startsWith("_port")) {
            setRoomFieldCommand(RPT_PORTABLE, R_PORTABLETYPE);
            return false;
        } else if (str.startsWith("_noport")) {
            setRoomFieldCommand(RPT_NOTPORTABLE, R_PORTABLETYPE);
            return false;
        } else if (str.startsWith("_ride")) {
            setRoomFieldCommand(RRT_RIDABLE, R_RIDABLETYPE);
            return false;
        } else if (str.startsWith("_noride")) {
            setRoomFieldCommand(RRT_NOTRIDABLE, R_RIDABLETYPE);
            return false;
        } else if (str.startsWith("_good")) {
            setRoomFieldCommand(RAT_GOOD, R_ALIGNTYPE);
            return false;
        } else if (str.startsWith("_neutral")) {
            setRoomFieldCommand(RAT_NEUTRAL, R_ALIGNTYPE);
            return false;
        } else if (str.startsWith("_evil")) {
            setRoomFieldCommand(RAT_EVIL, R_ALIGNTYPE);
            return false;
        } else if (str.startsWith("_rent")) {
            toggleRoomFlagCommand(RMF_RENT, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_shop")) {
            toggleRoomFlagCommand(RMF_SHOP, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_weaponshop")) {
            toggleRoomFlagCommand(RMF_WEAPONSHOP, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_armourshop")) {
            toggleRoomFlagCommand(RMF_ARMOURSHOP, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_foodshop")) {
            toggleRoomFlagCommand(RMF_FOODSHOP, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_petshop")) {
            toggleRoomFlagCommand(RMF_PETSHOP, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_guild")) {
            toggleRoomFlagCommand(RMF_GUILD, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_scoutguild")) {
            toggleRoomFlagCommand(RMF_SCOUTGUILD, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_mageguild")) {
            toggleRoomFlagCommand(RMF_MAGEGUILD, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_clericguild")) {
            toggleRoomFlagCommand(RMF_CLERICGUILD, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_warriorguild")) {
            toggleRoomFlagCommand(RMF_WARRIORGUILD, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_rangerguild")) {
            toggleRoomFlagCommand(RMF_RANGERGUILD, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_smob")) {
            toggleRoomFlagCommand(RMF_SMOB, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_quest")) {
            toggleRoomFlagCommand(RMF_QUEST, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_mob")) {
            toggleRoomFlagCommand(RMF_ANY, R_MOBFLAGS);
            return false;
        } else if (str.startsWith("_treasure")) {
            toggleRoomFlagCommand(RLF_TREASURE, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_armour")) {
            toggleRoomFlagCommand(RLF_ARMOUR, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_weapon")) {
            toggleRoomFlagCommand(RLF_WEAPON, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_water")) {
            toggleRoomFlagCommand(RLF_WATER, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_food")) {
            toggleRoomFlagCommand(RLF_FOOD, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_herb")) {
            toggleRoomFlagCommand(RLF_HERB, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_key")) {
            toggleRoomFlagCommand(RLF_KEY, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_mule")) {
            toggleRoomFlagCommand(RLF_MULE, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_horse")) {
            toggleRoomFlagCommand(RLF_HORSE, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_pack")) {
            toggleRoomFlagCommand(RLF_PACKHORSE, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_trained")) {
            toggleRoomFlagCommand(RLF_TRAINEDHORSE, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_rohirrim")) {
            toggleRoomFlagCommand(RLF_ROHIRRIM, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_warg")) {
            toggleRoomFlagCommand(RLF_WARG, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_boat")) {
            toggleRoomFlagCommand(RLF_BOAT, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_attention")) {
            toggleRoomFlagCommand(RLF_ATTENTION, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_watch")) {
            toggleRoomFlagCommand(RLF_TOWER, R_LOADFLAGS);
            return false;
        } else if (str.startsWith("_trollexit")) {
            m_trollExitMapping = !m_trollExitMapping;
            QByteArray toggleText = m_trollExitMapping ? "enabled" : "disabled";
            emit sendToUser("OK. Troll exit mapping is now " + toggleText + ".\r\n");
            sendPromptToUser();
            return false;
        } else if (str.startsWith("_note")) {
            setRoomFieldCommand(str.section(' ', 1), R_NOTE);
            return false;
        }

        if (str.startsWith("_open"))   {
            dooraction = true;
            daction = DAT_OPEN;
        } else if (str.startsWith("_close"))  {
            dooraction = true;
            daction = DAT_CLOSE;
        } else if (str.startsWith("_lock"))   {
            dooraction = true;
            daction = DAT_LOCK;
        } else if (str.startsWith("_unlock")) {
            dooraction = true;
            daction = DAT_UNLOCK;
        } else if (str.startsWith("_pick"))   {
            dooraction = true;
            daction = DAT_PICK;
        } else if (str.startsWith("_rock"))   {
            dooraction = true;
            daction = DAT_ROCK;
        } else if (str.startsWith("_bash"))   {
            dooraction = true;
            daction = DAT_BASH;
        } else if (str.startsWith("_break"))   {
            dooraction = true;
            daction = DAT_BREAK;
        } else if (str.startsWith("_block"))   {
            dooraction = true;
            daction = DAT_BLOCK;
        }

        if (str.startsWith("_search")) {
            QString pattern_str = str.section(' ', 1).trimmed();
            if (pattern_str.size() == 0) {
                emit sendToUser("Usage: _search [-(name|desc|note|exits|all)] pattern\r\n");
                return false;
            }
            RoomFilter f;
            if (!RoomFilter::parseRoomFilter(pattern_str, f))
                emit sendToUser(RoomFilter::parse_help);
            else
                searchCommand(f);
            return false;
        }
        if (str.startsWith("_dirs")) {
            QString pattern_str = str.section(' ', 1).trimmed();
            if (pattern_str.size() == 0) {
                emit sendToUser("Usage: _dirs [-(name|desc|note|exits|all)] pattern\r\n");
                return false;
            }
            RoomFilter f;
            if (!RoomFilter::parseRoomFilter(pattern_str, f))
                emit sendToUser(RoomFilter::parse_help);
            else
                dirsCommand(f);
            return false;
        }
        if (str.startsWith("_markcurrent")) {
            markCurrentCommand();
            emit sendToUser("--->Current room marked temporarily on the map.\r\n");
            sendPromptToUser();
            return false;
        }

        if (str == "_removedoornames") {
            m_mapData->removeDoorNames();
            emit sendToUser("OK. Secret exits purged.\r\n");
            sendPromptToUser();
        }
        if (str == "_back") {
            //while (!queue.isEmpty()) queue.dequeue();
            queue.clear();
            emit sendToUser("OK.\r\n");
            emit showPath(queue, true);
            sendPromptToUser();
            return false;
        }
        if (str == "_pdynamic") {
            printRoomInfo((1 << R_NAME) | (1 << R_DYNAMICDESC) | (1 << R_DESC));
            return false;
        }
        if (str == "_pstatic") {
            printRoomInfo((1 << R_NAME) | (1 << R_DESC));
            return false;
        }
        if (str == "_pnote") {
            printRoomInfo(1 << R_NOTE);
            return false;
        }
        if (str == "_help") {
            emit sendToUser("\r\nMMapper help:\r\n-------------\r\n");

            emit sendToUser("\r\nStandard MUD commands:\r\n");
            emit sendToUser("  Move commands: [n,s,...] or [north,south,...]\r\n");
            emit sendToUser("  Sync commands: [exa,l] or [examine,look]\r\n");

            emit sendToUser("\r\nManage prespammed command queue:\r\n");
            emit sendToUser("  _back        - delete prespammed commands from queue\r\n");

            emit sendToUser("\r\nDescription commands:\r\n");
            emit sendToUser("  _pdynamic    - prints current room description\r\n");
            emit sendToUser("  _pstatic     - the same as previous, but without moveable items\r\n");
            emit sendToUser("  _pnote       - print the note in the current room\r\n");

            emit sendToUser("\r\nHelp commands:\n");
            emit sendToUser("  _help      - this help text\r\n");
            emit sendToUser("  _maphelp   - help for mapping console commands\r\n");
            emit sendToUser("  _doorhelp  - help for door console commands\r\n");
            emit sendToUser("  _grouphelp - help for group manager console commands\r\n");

            emit sendToUser("\r\nOther commands:\n");
            emit sendToUser("  _vote                      - vote for MUME on TMC!\r\n");
            emit sendToUser("  _dirs [-options] pattern   - directions to matching rooms\r\n");
            emit sendToUser("  _search [-options] pattern - highlight matching rooms\r\n");
            emit sendToUser("  _markcurrent               - highlight the room you are currently in\r\n");
            emit sendToUser("  _time                      - display current MUME time\r\n");
            sendPromptToUser();
            return false;
        } else if (str == "_maphelp") {
            emit sendToUser("\r\nMMapper mapping help:\r\n-------------\r\n");
            emit sendToUser("\r\nExit commands:\r\n");
            emit sendToUser("  _name    [n,s,e,w,u,d] [name] - name a door in direction [dir] with [name]\r\n");
            emit sendToUser("  _door    [n,s,e,w,u,d]        - toggle a door in direction [dir]\r\n");
            emit sendToUser("  _hidden  [n,s,e,w,u,d]        - toggle a hidden door in direction [dir]\r\n");
            emit sendToUser("  _needkey [n,s,e,w,u,d]        - toggle a need key door in direction [dir]\r\n");
            emit sendToUser("  _noblock [n,s,e,w,u,d]        - toggle a no block door in direction [dir]\r\n");
            emit sendToUser("  _nobreak [n,s,e,w,u,d]        - toggle a no break door in direction [dir]\r\n");
            emit sendToUser("  _nopick  [n,s,e,w,u,d]        - toggle a no pick door in direction [dir]\r\n");
            emit sendToUser("  _delayed [n,s,e,w,u,d]        - toggle a delayed door in direction [dir]\r\n");

            emit sendToUser("\r\n");
            emit sendToUser("  _exit    [n,s,e,w,u,d]    - toggle a exit in direction [dir]\r\n");
            emit sendToUser("  _road    [n,s,e,w,u,d]    - toggle a road/trail exit in direction [dir]\r\n");
            emit sendToUser("  _climb   [n,s,e,w,u,d]    - toggle a climb exit in direction [dir]\r\n");
            emit sendToUser("  _random  [n,s,e,w,u,d]    - toggle a random exit in direction [dir]\r\n");
            emit sendToUser("  _special [n,s,e,w,u,d]    - toggle a special exit in direction [dir]\r\n");

            emit sendToUser("\r\nRoom flag commands:\r\n");
            emit sendToUser("  _port         - set the room to portable\r\n");
            emit sendToUser("  _noport       - set the room to not portable\r\n");
            emit sendToUser("  _dark         - set the room lighting to dark\r\n");
            emit sendToUser("  _lit          - set the room lighting to lit\r\n");
            emit sendToUser("  _nosundeath   - set the room to not cause troll/orc maluses\r\n");
            emit sendToUser("  _sundeath     - set the room to cause troll/orc maluses\r\n");
            emit sendToUser("  _ride         - set the room to ridable\r\n");
            emit sendToUser("  _noride       - set the room to not ridable\r\n");
            emit sendToUser("  _good         - set the room alignment to good\r\n");
            emit sendToUser("  _neutral      - set the room alignment to neutral\r\n");
            emit sendToUser("  _evil         - set the room alignment to evil\r\n");

            emit sendToUser("\r\n");
            emit sendToUser("  _rent         - toggle a rent mob in the room\r\n");
            emit sendToUser("  _shop         - toggle a shop in the room\r\n");
            emit sendToUser("  _weaponshop   - toggle a weapon shop in the room\r\n");
            emit sendToUser("  _armourshop   - toggle a armour shop in the room\r\n");
            emit sendToUser("  _foodshop     - toggle a food shop in the room\r\n");
            emit sendToUser("  _petshop      - toggle a pet shop in the room\r\n");
            emit sendToUser("  _guild        - toggle a guild in the room\r\n");
            emit sendToUser("  _scoutguild   - toggle a scout guild in the room\r\n");
            emit sendToUser("  _mageguild    - toggle a mage guild in the room\r\n");
            emit sendToUser("  _clericguild  - toggle a cleric guild in the room\r\n");
            emit sendToUser("  _warriorguild - toggle a warrior guild in the room\r\n");
            emit sendToUser("  _rangerguild  - toggle a ranger guild in the room\r\n");
            emit sendToUser("  _smob         - toggle a smob in the room\r\n");
            emit sendToUser("  _quest        - toggle a quest in the room\r\n");
            emit sendToUser("  _mob          - toggle a mob in the room\r\n");

            emit sendToUser("\r\n");
            emit sendToUser("  _treasure     - toggle a treasure in the room\r\n");
            emit sendToUser("  _armour       - toggle some armour in the room\r\n");
            emit sendToUser("  _weapon       - toggle some weapon in the room\r\n");
            emit sendToUser("  _water        - toggle some water in the room\r\n");
            emit sendToUser("  _food         - toggle some food in the room\r\n");
            emit sendToUser("  _herb         - toggle an herb in the room\r\n");
            emit sendToUser("  _key          - toggle a key in the room\r\n");
            emit sendToUser("  _mule         - toggle a mule in the room\r\n");
            emit sendToUser("  _horse        - toggle a horse in the room\r\n");
            emit sendToUser("  _pack         - toggle a packhorse in the room\r\n");
            emit sendToUser("  _trained      - toggle a trained horse in the room\r\n");
            emit sendToUser("  _rohirrim     - toggle a rohirrim horse in the room\r\n");
            emit sendToUser("  _warg         - toggle a warg in the room\r\n");
            emit sendToUser("  _boat         - toggle a boat in the room\r\n");
            emit sendToUser("  _attention    - toggle an attention flag in the room\r\n");
            emit sendToUser("  _watch        - toggle a watchable spot in the room\r\n");

            emit sendToUser("\r\nMiscellaneous commands:\r\n");
            emit sendToUser("  _note [note]         - set a note in the room\r\n");
            emit sendToUser("  _trollexit           - toggle troll-only exit mapping for direct sunlight\r\n");

            emit sendToUser("\r\n");

            sendPromptToUser();
            return false;
        } else if (str == "_doorhelp") {
            emit sendToUser("\r\nMMapper door help:\r\n-------------\r\n");
            emit sendToUser("\r\nDoor commands:\r\n");
            emit sendToUser("  _open   [n,s,e,w,u,d]   - open door [dir]\r\n");
            emit sendToUser("  _close  [n,s,e,w,u,d]   - close door [dir]\r\n");
            emit sendToUser("  _lock   [n,s,e,w,u,d]   - lock door [dir]\r\n");
            emit sendToUser("  _unlock [n,s,e,w,u,d]   - unlock door [dir]\r\n");
            emit sendToUser("  _pick   [n,s,e,w,u,d]   - pick door [dir]\r\n");
            emit sendToUser("  _rock   [n,s,e,w,u,d]   - throw rock door [dir]\r\n");
            emit sendToUser("  _bash   [n,s,e,w,u,d]   - bash door [dir]\r\n");
            emit sendToUser("  _break  [n,s,e,w,u,d]   - cast 'break door' door [dir]\r\n");
            emit sendToUser("  _block  [n,s,e,w,u,d]   - cast 'block door' door [dir]\r\n");

            emit sendToUser("\r\nDoor variables:\r\n");
            emit sendToUser("  $$DOOR_N$$   - secret name of door leading north\r\n");
            emit sendToUser("  $$DOOR_S$$   - secret name of door leading south\r\n");
            emit sendToUser("  $$DOOR_E$$   - secret name of door leading east\r\n");
            emit sendToUser("  $$DOOR_W$$   - secret name of door leading west\r\n");
            emit sendToUser("  $$DOOR_U$$   - secret name of door leading up\r\n");
            emit sendToUser("  $$DOOR_D$$   - secret name of door leading down\r\n");
            emit sendToUser("  $$DOOR$$     - the same as 'exit'\r\n");

            emit sendToUser("\r\nDestructive commands:\r\n");
            emit sendToUser("  _removedoornames   - removes all secret door names from the current map\r\n");

            emit sendToUser("\r\n");

            sendPromptToUser();
            return false;
        } else if (str == "_grouphelp") {
            emit sendToUser("\r\nMMapper group manager help:\r\n-------------\r\n");
            emit sendToUser("\r\nGroup commands:\r\n");
            emit sendToUser("  _gt [message]     - send a grouptell with the [message]\r\n");

            emit sendToUser("\r\n");

            sendPromptToUser();
            return false;
        }
    }

    if (dooraction) {
        if (str.endsWith(" n"))
            performDoorCommand(NORTH, daction);
        else if (str.endsWith(" s"))
            performDoorCommand(SOUTH, daction);
        else if (str.endsWith(" e"))
            performDoorCommand(EAST, daction);
        else if (str.endsWith(" w"))
            performDoorCommand(WEST, daction);
        else if (str.endsWith(" u"))
            performDoorCommand(UP, daction);
        else if (str.endsWith(" d"))
            performDoorCommand(DOWN, daction);
        else
            performDoorCommand(UNKNOWN, daction);
        return false;
    }

    if (str == "n" || str == "north") {
        queue.enqueue(CID_NORTH);
        emit showPath(queue, true);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_NORTH);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "s" || str == "south") {
        queue.enqueue(CID_SOUTH);
        emit showPath(queue, true);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_SOUTH);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "e" || str == "east") {
        queue.enqueue(CID_EAST);
        emit showPath(queue, true);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_EAST);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "w" || str == "west") {
        queue.enqueue(CID_WEST);
        emit showPath(queue, true);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_WEST);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "u" || str == "up") {
        queue.enqueue(CID_UP);
        emit showPath(queue, true);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_UP);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "d" || str == "down") {
        queue.enqueue(CID_DOWN);
        emit showPath(queue, true);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_DOWN);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "l" || str == "look") {
        queue.enqueue(CID_LOOK);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_LOOK);
            return false; //do not send command to mud server for offline mode
        }
    }
    if (str == "exa" || str == "examine") {
        queue.enqueue(CID_LOOK);
        if (Config().m_mapMode != 2)
            return true;
        else {
            offlineCharacterMove(CID_LOOK);
            return false; //do not send command to mud server for offline mode
        }
    }
    /*if (str.startsWith("scout")) {
    queue.enqueue(CID_SCOUT);
    if (Config().m_mapMode != 2)
    return true;
    }*/
    if ((str == "f" || str == "flee") && Config().m_mapMode == 2) {
        offlineCharacterMove(CID_FLEE);
        return false; //do not send command to mud server for offline mode
    }

    if (Config().m_mapMode != 2)
        return true;
    else {
        sendPromptToUser();
        return false; //do not send command to mud server for offline mode
    }
}

void AbstractParser::offlineCharacterMove(CommandIdType direction)
{
    bool flee = false;
    if (direction == CID_FLEE) {
        flee = true;
        emit sendToUser("You flee head over heels!\r\n");
        direction = (CommandIdType) (rand() % 6);
    }

    if (!flee) queue.dequeue();

    Coordinate c;
    c = m_mapData->getPosition();
    const RoomSelection *rs1 = m_mapData->select(c);
    const Room *rb = rs1->values().front();

    if (direction == CID_LOOK) {
        sendRoomInfoToUser(rb);
        sendRoomExitsInfoToUser(rb);
        sendPromptToUser(rb);
    } else {
        const Exit &e = rb->exit((uint)direction);
        if ((Mmapper2Exit::getFlags(e) & EF_EXIT) && (e.outBegin() != e.outEnd())) {
            const RoomSelection *rs2 = m_mapData->select();
            const Room *r = m_mapData->getRoom(*e.outBegin(), rs2);

            sendRoomInfoToUser(r);
            sendRoomExitsInfoToUser(r);
            sendPromptToUser(r);
            // Create character move event for main move/search algorithm
            emit event(Mmapper2Event::createEvent(direction, Mmapper2Room::getName(r),
                                                  Mmapper2Room::getDynamicDescription(r),
                                                  Mmapper2Room::getDescription(r), 0, 0, 0));
            emit showPath(queue, true);
            m_mapData->unselect(rs2);
        } else {
            if (!flee)
                emit sendToUser("You cannot go that way...");
            else
                emit sendToUser("You cannot flee!!!");
            sendPromptToUser(rb);
        }
    }
    m_mapData->unselect(rs1);
    usleep(40000);
}

void AbstractParser::sendRoomInfoToUser(const Room *r)
{
    if (!r) return;
    QByteArray roomName("\r\n");
    if (!Config().m_roomNameColor.isEmpty()) {
        roomName += "\033" + Config().m_roomNameColor.toLatin1();
    }
    roomName += Mmapper2Room::getName(r).toLatin1() + "\033[0m\r\n";
    emit sendToUser(roomName);

    QByteArray roomDescription;
    if (!Config().m_roomDescColor.isEmpty()) {
        roomDescription += "\033" + Config().m_roomDescColor.toLatin1();
    }
    roomDescription += Mmapper2Room::getDescription(r).toLatin1() + "\033[0m";
    roomDescription.replace("\n", "\r\n");
    emit sendToUser(roomDescription);

    emit sendToUser(Mmapper2Room::getDynamicDescription(r).toLatin1().replace("\n", "\r\n"));
}

void AbstractParser::sendRoomExitsInfoToUser(const Room *r)
{
    if (!r) return;
    char sunCharacter = (m_mumeClock->getMumeMoment().toTimeOfDay() <= TIME_DAY) ? '*' : '^';
    const RoomSelection *rs = m_mapData->select();

    QString etmp = "Exits/emulated:";
    int j;
    for (int jj = 0; jj < 7; jj++) {
        switch (jj) {
        case 1:
            j = 2;
            break;
        case 2:
            j = 1;
            break;
        default:
            j = jj;
            break;
        }

        bool door = false;
        bool exit = false;
        bool road = false;
        bool trail = false;
        bool climb = false;
        bool oneway = false;
        bool sundeath = false;
        bool swim = false;

        const Exit &e = r->exit(j);
        if (ISSET(Mmapper2Exit::getFlags(e), EF_EXIT)) {
            exit = true;
            etmp += " ";

            RoomTerrainType sourceRoomTerrainType = Mmapper2Room::getTerrainType(r);
            if (e.outBegin() != e.outEnd()) {
                uint targetId = *e.outBegin();
                const Room *targetRoom = m_mapData->getRoom(targetId, rs);
                RoomTerrainType targetRoomTerrainType = Mmapper2Room::getTerrainType(targetRoom);

                // Sundeath exit flag modifiers
                if (Mmapper2Room::getSundeathType(targetRoom) == RST_SUNDEATH) {
                    sundeath = true;
                    etmp += sunCharacter;
                }

                // Terrain type exit modifiers
                if (targetRoomTerrainType == RTT_RAPIDS || targetRoomTerrainType == RTT_UNDERWATER) {
                    swim = true;
                    etmp += "~";

                } else if (targetRoomTerrainType == RTT_ROAD && sourceRoomTerrainType == RTT_ROAD) {
                    road = true;
                    etmp += "=";
                }
            }

            if (!road && ISSET(Mmapper2Exit::getFlags(e), EF_ROAD)) {
                if (sourceRoomTerrainType == RTT_ROAD) {
                    road = true;
                    etmp += "=";
                } else {
                    trail = true;
                    etmp += "-";
                }
            }

            if (ISSET(Mmapper2Exit::getFlags(e), EF_DOOR)) {
                door = true;
                etmp += "{";
            } else if (ISSET(Mmapper2Exit::getFlags(e), EF_CLIMB)) {
                climb = true;
                etmp += "|";
            }

            switch (j) {
            case 0:
                etmp += "north";
                break;
            case 1:
                etmp += "south";
                break;
            case 2:
                etmp += "east";
                break;
            case 3:
                etmp += "west";
                break;
            case 4:
                etmp += "up";
                break;
            case 5:
                etmp += "down";
                break;
            case 6:
                etmp += "unknown";
                break;
            }
        }

        if (door) etmp += "}";
        else if (climb) etmp += "|";
        if (road) etmp += "=";
        else if (trail) etmp += "-";
        if (sundeath) etmp += sunCharacter;
        if (exit) etmp += ",";
    }
    etmp = etmp.left(etmp.length() - 1);
    etmp += ".";

    QByteArray cn = enhanceExits(r);
    m_mapData->unselect(rs);

    emit sendToUser(etmp.toLatin1() + cn);
}

void AbstractParser::sendPromptToUser()
{
    if (!m_lastPrompt.isEmpty() && Config().m_mapMode != 2) {
        emit sendToUser(m_lastPrompt.toLatin1());
        return ;
    }

    // Emulate prompt mode
    Coordinate c = getPosition();
    const RoomSelection *rs = m_mapData->select();
    const Room *r = m_mapData->getRoom(c, rs);

    sendPromptToUser(r);

    m_mapData->unselect(rs);
}

void AbstractParser::sendPromptToUser(const Room *r)
{


    char light = (Mmapper2Room::getLightType(r) == RLT_DARK) ? 'o' : '*';

    char terrain = ' ';
    switch (Mmapper2Room::getTerrainType(r)) {
    case RTT_INDOORS:
        terrain = '[';
        break;   // [  // indoors
    case RTT_CITY:
        terrain = '#';
        break;      // #  // city
    case RTT_FIELD:
        terrain = '.';
        break;     // .  // field
    case RTT_FOREST:
        terrain = 'f';
        break;    // f  // forest
    case RTT_HILLS:
        terrain = '(';
        break;     // (  // hills
    case RTT_MOUNTAINS:
        terrain = '<';
        break; // <  // mountains
    case RTT_SHALLOW:
        terrain = '%';
        break;   // %  // shallow
    case RTT_WATER:
        terrain = '~';
        break;     // ~  // water
    case RTT_RAPIDS:
        terrain = 'W';
        break;    // W  // rapids
    case RTT_UNDERWATER:
        terrain = 'U';
        break;// U  // underwater
    case RTT_ROAD:
        terrain = '+';
        break;      // +  // road
    case RTT_TUNNEL:
        terrain = '=';
        break;    // =  // tunnel
    case RTT_CAVERN:
        terrain = 'O';
        break;    // O  // cavern
    case RTT_BRUSH:
        terrain = ':';
        break;     // :  // brush
    default:
        break;
    };

    QByteArray prompt("\r\n");
    prompt += light;
    prompt += terrain;
    prompt += ">";
    emit sendToUser(prompt);
}

void AbstractParser::performDoorCommand(DirectionType direction, DoorActionType action)
{
    QByteArray cn = emptyByteArray;
    QByteArray dn = emptyByteArray;

    switch (action) {
    case DAT_OPEN:
        cn = "open ";
        break;
    case DAT_CLOSE:
        cn = "close ";
        break;
    case DAT_LOCK:
        cn = "lock ";
        break;
    case DAT_UNLOCK:
        cn = "unlock ";
        break;
    case DAT_PICK:
        cn = "pick ";
        break;
    case DAT_ROCK:
        cn = "throw rock ";
        break;
    case DAT_BASH:
        cn = "bash ";
        break;
    case DAT_BREAK:
        cn = "cast 'break door' ";
        break;
    case DAT_BLOCK:
        cn = "cast 'block door' ";
        break;
    default:
        break;
    }

    Coordinate c = getPosition();

    dn = m_mapData->getDoorName(c, direction).toLatin1();

    bool needdir = false;

    if (dn == emptyByteArray) {
        dn = "exit";
        needdir = true;
    } else
        for (int i = 0; i < 6; i++) {
            if ( (((DirectionType)i) != direction)
                    && (m_mapData->getDoorName(c, (DirectionType)i).toLatin1() == dn) )
                needdir = true;
        }

    switch (direction) {
    case NORTH:
        if (needdir)
            cn += dn + " n\r\n";
        else
            cn += dn + "\r\n";
        break;
    case SOUTH:
        if (needdir)
            cn += dn + " s\r\n";
        else
            cn += dn + "\r\n";
        break;
    case EAST:
        if (needdir)
            cn += dn + " e\r\n";
        else
            cn += dn + "\r\n";
        break;
    case WEST:
        if (needdir)
            cn += dn + " w\r\n";
        else
            cn += dn + "\r\n";
        break;
    case UP:
        if (needdir)
            cn += dn + " u\r\n";
        else
            cn += dn + "\r\n";
        break;
    case DOWN:
        if (needdir)
            cn += dn + " d\r\n";
        else
            cn += dn + "\r\n";
        break;
    default:
        cn += dn + "\r\n";
        break;
    }

    if (Config().m_mapMode != 2) { // online mode
        emit sendToMud(cn);
        emit sendToUser("--->" + cn);
    } else {
        emit sendToUser("--->" + cn);
        emit sendToUser("OK.\r\n");
    }
    sendPromptToUser();
}

void AbstractParser::genericDoorCommand(QString command, DirectionType direction)
{
    QByteArray cn = emptyByteArray;
    QByteArray dn = emptyByteArray;

    Coordinate c = getPosition();

    dn = m_mapData->getDoorName(c, direction).toLatin1();

    bool needdir = false;

    if (dn == emptyByteArray) {
        dn = "exit";
        needdir = true;
    } else
        for (int i = 0; i < 6; i++) {
            if ( (((DirectionType)i) != direction)
                    && (m_mapData->getDoorName(c, (DirectionType)i).toLatin1() == dn) )
                needdir = true;
        }

    switch (direction) {
    case NORTH:
        if (needdir)
            cn += dn + " n\r\n";
        else
            cn += dn + "\r\n";
        command = command.replace("$$DOOR_N$$", cn);
        break;
    case SOUTH:
        if (needdir)
            cn += dn + " s\r\n";
        else
            cn += dn + "\r\n";
        command = command.replace("$$DOOR_S$$", cn);
        break;
    case EAST:
        if (needdir)
            cn += dn + " e\r\n";
        else
            cn += dn + "\r\n";
        command = command.replace("$$DOOR_E$$", cn);
        break;
    case WEST:
        if (needdir)
            cn += dn + " w\r\n";
        else
            cn += dn + "\r\n";
        command = command.replace("$$DOOR_W$$", cn);
        break;
    case UP:
        if (needdir)
            cn += dn + " u\r\n";
        else
            cn += dn + "\r\n";
        command = command.replace("$$DOOR_U$$", cn);
        break;
    case DOWN:
        if (needdir)
            cn += dn + " d\r\n";
        else
            cn += dn + "\r\n";
        command = command.replace("$$DOOR_D$$", cn);
        break;
    default:
        cn += dn + "\r\n";
        command = cn;
        break;
    }

    if (Config().m_mapMode != 2) { // online mode
        emit sendToMud(command.toLatin1());
        emit sendToUser("--->" + command.toLatin1());
    } else {
        emit sendToUser("--->" + command.toLatin1());
        emit sendToUser("OK.\r\n");
    }
    sendPromptToUser();
}

void AbstractParser::nameDoorCommand(QString doorname, DirectionType direction)
{
    Coordinate c = getPosition();

    //if (doorname.isEmpty()) toggleExitFlagCommand(EF_DOOR, direction);
    m_mapData->setDoorName(c, doorname, direction);
    emit sendToUser("--->Doorname set to: " + doorname.toLatin1() + "\r\n");
    sendPromptToUser();
}

void AbstractParser::toggleExitFlagCommand(uint flag, DirectionType direction)
{
    Coordinate c = getPosition();

    m_mapData->toggleExitFlag(c, flag, direction, E_FLAGS);

    QString toggle, flagname;
    if (m_mapData->getExitFlag(c, flag, direction, E_FLAGS))
        toggle = "enabled";
    else
        toggle = "disabled";

    switch (flag) {
    case EF_EXIT:
        flagname = "Possible";
        break;
    case EF_DOOR:
        flagname = "Door";
        break;
    case EF_ROAD:
        flagname = "Road";
        break;
    case EF_CLIMB:
        flagname = "Climbable";
        break;
    case EF_RANDOM:
        flagname = "Random";
        break;
    case EF_SPECIAL:
        flagname = "Special";
        break;
    default:
        flagname = "Unknown";
        break;
    }

    emit sendToUser("--->" + flagname.toLatin1() + " exit " + toggle.toLatin1() + "\r\n");
    sendPromptToUser();
}

void AbstractParser::toggleDoorFlagCommand(uint flag, DirectionType direction)
{
    Coordinate c = getPosition();

    m_mapData->toggleExitFlag(c, flag, direction, E_DOORFLAGS);

    QString toggle, flagname;
    if (m_mapData->getExitFlag(c, flag, direction, E_DOORFLAGS))
        toggle = "enabled";
    else
        toggle = "disabled";

    switch (flag) {
    case DF_HIDDEN:
        flagname = "Hidden";
        break;
    case DF_NEEDKEY:
        flagname = "Need key";
        break;
    case DF_NOBLOCK:
        flagname = "No block";
        break;
    case DF_NOBREAK:
        flagname = "No break";
        break;
    case DF_NOPICK:
        flagname = "No pick";
        break;
    case DF_DELAYED:
        flagname = "Delayed";
        break;
    default:
        flagname = "Unknown";
        break;
    }

    emit sendToUser("--->" + flagname.toLatin1() + " door " + toggle.toLatin1() + "\r\n");
    sendPromptToUser();
}

void AbstractParser::setRoomFieldCommand(const QVariant &flag, uint field)
{
    Coordinate c = getPosition();

    m_mapData->setRoomField(c, flag, field);

    emit sendToUser("--->Room field set\r\n");
    sendPromptToUser();
}

void AbstractParser::toggleRoomFlagCommand(uint flag, uint field)
{
    Coordinate c = getPosition();

    m_mapData->toggleRoomFlag(c, flag, field);

    QString toggle;
    if (m_mapData->getRoomFlag(c, flag, field))
        toggle = "enabled";
    else
        toggle = "disabled";

    emit sendToUser("--->Room flag " + toggle.toLatin1() + "\r\n");
    sendPromptToUser();
}

void AbstractParser::printRoomInfo(uint fieldset)
{
    Coordinate c = getPosition();

    const RoomSelection *rs = m_mapData->select(c);
    const Room *r = rs->values().front();

    QString result;

    if (fieldset & (1 << R_NAME)) result = result + (*r)[R_NAME].toString() + "\r\n";
    if (fieldset & (1 << R_DESC)) result = result + (*r)[R_DESC].toString();
    if (fieldset & (1 << R_DYNAMICDESC)) result = result + (*r)[R_DYNAMICDESC].toString();
    if (fieldset & (1 << R_NOTE)) result = result + "Note: " + (*r)[R_NOTE].toString() + "\r\n";

    emit sendToUser(result.toLatin1());
    sendPromptToUser();
    m_mapData->unselect(rs);
}

void AbstractParser::sendGTellToUser(const QByteArray &ba)
{
    emit sendToUser(ba);
    sendPromptToUser();
}

