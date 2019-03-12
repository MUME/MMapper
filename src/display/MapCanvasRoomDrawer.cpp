/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#include "MapCanvasRoomDrawer.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <set>
#include <stdexcept>
#include <QMessageLogContext>
#include <QtGui/qopengl.h>
#include <QtGui>

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/Color.h"
#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"
#include "../global/utils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/enums.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "ConnectionLineBuilder.h"
#include "FontFormatFlags.h"
#include "MapCanvasData.h"
#include "OpenGL.h"
#include "RoadIndex.h"

// TODO: Make all of the WALL_COLOR_XXX configurable?
// Also, should FALL and CLIMB damage be separate colors? What about wall and door?
static const auto WALL_COLOR_CLIMB = QColor::fromRgbF(0.7, 0.7, 0.7);       // lightgray;
static const auto WALL_COLOR_FALL_DAMAGE = QColor::fromRgbF(0.0, 1.0, 1.0); // cyan
static const auto WALL_COLOR_GUARDED = QColor::fromRgbF(1.0, 1.0, 0.0);     // yellow;
static const auto WALL_COLOR_NO_FLEE = Qt::black;
static const auto WALL_COLOR_NO_MATCH = Qt::blue;
static const auto WALL_COLOR_NOTMAPPED = QColor::fromRgbF(1.0, 0.5, 0.0); // orange
static const auto WALL_COLOR_RANDOM = QColor::fromRgbF(1.0, 0.0, 0.0);    // red
static const auto WALL_COLOR_SPECIAL = QColor::fromRgbF(0.8, 0.1, 0.8);   // lightgreen
static const auto WALL_COLOR_WALL_DOOR = QColor::fromRgbF(0.2, 0.0, 0.0); // very dark red

static bool isOdd(const ExitDirection sourceDir)
{
    return static_cast<int>(sourceDir) % 2 == 1;
}

static QMatrix4x4 getTranslationMatrix(float x, float y)
{
    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(x, y, 0);
    return model;
}
static QMatrix4x4 getTranslationMatrix(const int x, const int y)
{
    return getTranslationMatrix(static_cast<float>(x), static_cast<float>(y));
}

static QColor getInfoMarkColor(InfoMarkType infoMarkType, InfoMarkClass infoMarkClass)
{
    const QColor defaultColor = (infoMarkType == InfoMarkType::TEXT)
                                    ? QColor(0, 0, 0, 76)         // Black
                                    : QColor(255, 255, 255, 178); // White
    switch (infoMarkClass) {
    case InfoMarkClass::HERB:
        return QColor(0, 255, 0, 140); // Green
    case InfoMarkClass::RIVER:
        return QColor(76, 216, 255, 140); // Cyan-like
    case InfoMarkClass::MOB:
        return QColor(255, 0, 0, 140); // Red
    case InfoMarkClass::COMMENT:
        return QColor(192, 192, 192, 140); // Light grey
    case InfoMarkClass::ROAD:
        return QColor(140, 83, 58, 140); // Maroonish
    case InfoMarkClass::OBJECT:
        return QColor(255, 255, 0, 140); // Yellow

    case InfoMarkClass::GENERIC:
    case InfoMarkClass::PLACE:
    case InfoMarkClass::ACTION:
    case InfoMarkClass::LOCALITY:
        return defaultColor;
    }

    return defaultColor;
}

static FontFormatFlags getFontFormatFlags(InfoMarkClass infoMarkClass)
{
    switch (infoMarkClass) {
    case InfoMarkClass::GENERIC:
    case InfoMarkClass::HERB:
    case InfoMarkClass::RIVER:
    case InfoMarkClass::PLACE:
    case InfoMarkClass::MOB:
    case InfoMarkClass::COMMENT:
    case InfoMarkClass::ROAD:
    case InfoMarkClass::OBJECT:
        break;

    case InfoMarkClass::ACTION:
        return FontFormatFlags::ITALICS;

    case InfoMarkClass::LOCALITY:
        return FontFormatFlags::UNDERLINE;
    }
    return FontFormatFlags::NONE;
}

// TODO: change to std::optional<QColor> in C++17 instead of returning transparent
static QColor getWallColor(const ExitFlags &flags)
{
    const auto drawNoMatchExits = getConfig().canvas.drawNoMatchExits;

    if (flags.isNoFlee()) {
        return WALL_COLOR_NO_FLEE;
    } else if (flags.isRandom()) {
        return WALL_COLOR_RANDOM;
    } else if (flags.isFall() || flags.isDamage()) {
        return WALL_COLOR_FALL_DAMAGE;
    } else if (flags.isSpecial()) {
        return WALL_COLOR_SPECIAL;
    } else if (flags.isClimb()) {
        return WALL_COLOR_CLIMB;
    } else if (flags.isGuarded()) {
        return WALL_COLOR_GUARDED;
    } else if (drawNoMatchExits && flags.isNoMatch()) {
        return WALL_COLOR_NO_MATCH;
    } else {
        return Qt::transparent;
    }
}

// REVISIT: merge this with getWallColor()?
static QColor getVerticalColor(const ExitFlags &flags, const QColor &noFleeColor)
{
    // REVISIT: is it a bug that the NO_FLEE and NO_MATCH colors have 100% opacity?
    if (flags.isNoFlee()) {
        return noFleeColor;
    } else if (flags.isClimb()) {
        // NOTE: This color is slightly darker than WALL_COLOR_CLIMB
        return QColor::fromRgbF(0.5, 0.5, 0.5); // lightgray
    } else {
        return getWallColor(flags);
    }
}

static XColor4f getWallExitColor(const qint32 layer)
{
    if (layer == 0)
        return XColor4f{Qt::black};
    else if (layer > 0)
        return XColor4f{0.3f, 0.3f, 0.3f, 0.6f};
    else
        return XColor4f{Qt::black, std::max(0.0f, 0.5f - 0.03f * static_cast<float>(layer))};
}

static XColor4f getRoomColor(const qint32 layer, bool isDark = false, bool hasNoSunDeath = false)
{
    if (layer > 0)
        return XColor4f{0.3f, 0.3f, 0.3f, std::max(0.0f, 0.6f - 0.2f * static_cast<float>(layer))};

    const float alpha = (layer < 0) ? 1.0f : 0.9f;
    if (isDark)
        return XColor4f{0.63f, 0.58f, 0.58f, alpha};
    else if (hasNoSunDeath) {
        return XColor4f{0.83f, 0.78f, 0.78f, alpha};
    }
    return XColor4f{Qt::white, alpha};
}

static QString getDoorPostFix(const Room *const room, const ExitDirection dir)
{
    static constexpr const auto SHOWN_FLAGS = DoorFlag::NEED_KEY | DoorFlag::NO_PICK
                                              | DoorFlag::DELAYED;

    const DoorFlags flags = room->exit(dir).getDoorFlags();
    if (!flags.containsAny(SHOWN_FLAGS))
        return QString{};

    return QString::asprintf(" [%s%s%s]",
                             flags.needsKey() ? "L" : "",
                             flags.isNoPick() ? "/NP" : "",
                             flags.isDelayed() ? "d" : "");
}

static QString getPostfixedDoorName(const Room *const room, const ExitDirection dir)
{
    const auto postFix = getDoorPostFix(room, dir);
    return room->exit(dir).getDoorName() + postFix;
}

void MapCanvasRoomDrawer::drawInfoMarks()
{
    MarkerListIterator m(m_mapCanvasData.m_data->getMarkersList());
    while (m.hasNext()) {
        drawInfoMark(m.next());
    }
}

void MapCanvasRoomDrawer::drawInfoMark(InfoMark *marker)
{
    const float x1 = static_cast<float>(marker->getPosition1().x) / INFOMARK_SCALE;
    const float y1 = static_cast<float>(marker->getPosition1().y) / INFOMARK_SCALE;
    const int layer = marker->getPosition1().z;
    float x2 = static_cast<float>(marker->getPosition2().x) / INFOMARK_SCALE;
    float y2 = static_cast<float>(marker->getPosition2().y) / INFOMARK_SCALE;
    const float dx = x2 - x1;
    const float dy = y2 - y1;

    float width = 0;
    float height = 0;

    if (layer != m_currentLayer) {
        return;
    }

    const auto infoMarkType = marker->getType();
    const auto infoMarkClass = marker->getClass();

    // Color depends of the class of the InfoMark
    const QColor color = getInfoMarkColor(infoMarkType, infoMarkClass);
    const auto fontFormatFlag = getFontFormatFlags(infoMarkClass);

    if (infoMarkType == InfoMarkType::TEXT) {
        width = getScaledFontWidth(marker->getText(), fontFormatFlag);
        height = getScaledFontHeight();
        x2 = x1 + width;
        y2 = y1 + height;

        // Update the text marker's X2 and Y2 position
        // REVISIT: This should be done in the "data" stage
        marker->setPosition2(Coordinate{static_cast<int>(x2 * INFOMARK_SCALE),
                                        static_cast<int>(y2 * INFOMARK_SCALE),
                                        marker->getPosition2().z});
    }

    // check if marker is visible
    if (((x1 + 1.0f < m_visible1.x) || (x1 - 1.0f > m_visible2.x + 1.0f))
        && ((x2 + 1.0f < m_visible1.x) || (x2 - 1.0f > m_visible2.x + 1))) {
        return;
    }
    if (((y1 + 1.0f < m_visible1.y) || (y1 - 1.0f > m_visible2.y + 1.0f))
        && ((y2 + 1.0f < m_visible1.y) || (y2 - 1.0f > m_visible2.y + 1.0f))) {
        return;
    }

    m_opengl.glPushMatrix();
    m_opengl.glTranslatef(x1, y1, 0.0f);

    switch (infoMarkType) {
    case InfoMarkType::TEXT:
        // Render background
        m_opengl.apply(XColor4f{color});
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.draw(DrawType::TRIANGLE_STRIP,
                      std::vector<Vec3f>{
                          Vec3f{0.0f, 0.0f, 1.0f},
                          Vec3f{0.0f, 0.25f + height, 1.0f},
                          Vec3f{0.2f + width, 0.0f, 1.0f},
                          Vec3f{0.2f + width, 0.25f + height, 1.0f},
                      });
        m_opengl.apply(XDisable{XOption::BLEND});

        // Render text proper
        m_opengl.glTranslatef(-x1 / 2.0f, -y1 / 2.0f, 0.0f);
        renderText(x1 + 0.1f,
                   y1 + 0.3f,
                   marker->getText(),
                   textColor(color),
                   fontFormatFlag,
                   marker->getRotationAngle());
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        break;
    case InfoMarkType::LINE:
        m_opengl.apply(XColor4f{color});
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XDevicePointSize{2.0});
        m_opengl.apply(XDeviceLineWidth{2.0});
        m_opengl.draw(DrawType::LINES,
                      std::vector<Vec3f>{
                          Vec3f{0.0f, 0.0f, 0.1f},
                          Vec3f{dx, dy, 0.1f},
                      });
        m_opengl.apply(XDisable{XOption::BLEND});
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        break;
    case InfoMarkType::ARROW:
        m_opengl.apply(XColor4f{color});
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XDevicePointSize{2.0});
        m_opengl.apply(XDeviceLineWidth{2.0});
        m_opengl.draw(DrawType::LINE_STRIP,
                      std::vector<Vec3f>{Vec3f{0.0f, 0.05f, 1.0f},
                                         Vec3f{dx - 0.2f, dy + 0.1f, 1.0f},
                                         Vec3f{dx - 0.1f, dy + 0.1f, 1.0f}});
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{Vec3f{dx - 0.1f, dy + 0.1f - 0.07f, 1.0f},
                                         Vec3f{dx - 0.1f, dy + 0.1f + 0.07f, 1.0f},
                                         Vec3f{dx + 0.1f, dy + 0.1f, 1.0f}});
        m_opengl.apply(XDisable{XOption::BLEND});
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        break;
    }

    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::alphaOverlayTexture(QOpenGLTexture *texture)
{
    if (texture != nullptr) {
        texture->bind();
    }
    m_opengl.callList(m_gllist.room);
}

void MapCanvasRoomDrawer::drawRoomDoorName(const Room *const sourceRoom,
                                           const ExitDirection sourceDir,
                                           const Room *const targetRoom,
                                           const ExitDirection targetDir)
{
    assert(sourceRoom != nullptr);
    assert(targetRoom != nullptr);

    const Coordinate sourcePos = sourceRoom->getPosition();
    const auto srcX = sourcePos.x;
    const auto srcY = sourcePos.y;
    const auto srcZ = sourcePos.z;

    const Coordinate targetPos = targetRoom->getPosition();
    const auto tarX = targetPos.x;
    const auto tarY = targetPos.y;
    const auto tarZ = targetPos.z;

    if (srcZ != m_currentLayer && tarZ != m_currentLayer) {
        return;
    }

    const auto dX = srcX - tarX;
    const auto dY = srcY - tarY;

    QString name;
    bool together = false;

    if (targetRoom->exit(targetDir).isDoor()
        // the other room has a door?
        && targetRoom->exit(targetDir).hasDoorName()
        // has a door on both sides...
        && targetRoom->exit(targetDir).isHiddenExit()
        // is hidden
        && std::abs(dX) <= 1 && std::abs(dY) <= 1) { // the door is close by!
        // skip the other direction since we're printing these together
        if (isOdd(sourceDir)) {
            return;
        }

        together = true;

        // no need for duplicating names (its spammy)
        const QString sourceName = getPostfixedDoorName(sourceRoom, sourceDir);
        const QString targetName = getPostfixedDoorName(targetRoom, targetDir);
        if (sourceName != targetName) {
            name = sourceName + "/" + targetName;
        } else {
            name = sourceName;
        }
    } else {
        name = getPostfixedDoorName(sourceRoom, sourceDir);
    }

    const float width = getScaledFontWidth(name);
    const float height = getScaledFontHeight();

    float boxX = 0, boxY = 0;
    if (together) {
        boxX = srcX - (width / 2.0f) - (dX * 0.5f);
        boxY = srcY - 0.5f - (dY * 0.5f);
    } else {
        boxX = srcX - (width / 2.0f);
        switch (sourceDir) {
        case ExitDirection::NORTH:
            boxY = srcY - 0.65f;
            break;
        case ExitDirection::SOUTH:
            boxY = srcY - 0.15f;
            break;
        case ExitDirection::WEST:
            boxY = srcY - 0.5f;
            break;
        case ExitDirection::EAST:
            boxY = srcY - 0.35f;
            break;
        case ExitDirection::UP:
            boxY = srcY - 0.85f;
            break;
        case ExitDirection::DOWN:
            boxY = srcY;
            break;

        case ExitDirection::UNKNOWN:
        case ExitDirection::NONE:
        default:
            break;
        };
    }

    drawTextBox(name, boxX, boxY, width, height);
}

void MapCanvasRoomDrawer::drawTextBox(
    const QString &name, const float x, const float y, const float width, const float height)
{
    const float boxX2 = x + width;
    const float boxY2 = y + height;

    // check if box is visible
    if (((x + 1.0f < m_visible1.x) || (x - 1 > m_visible2.x + 1))
        && ((boxX2 + 1 < m_visible1.x) || (boxX2 - 1.0f > m_visible2.x + 1.0f))) {
        return;
    }
    if (((y + 1.0f < m_visible1.y) || (y - 1 > m_visible2.y + 1))
        && ((boxY2 + 1.0f < m_visible1.y) || (boxY2 - 1.0f > m_visible2.y + 1.0f))) {
        return;
    }

    m_opengl.glPushMatrix();

    m_opengl.glTranslatef(x /*-0.5*/, y /*-0.5*/, 0);

    // Render background
    m_opengl.apply(XColor4f{0, 0, 0, 0.3f});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.draw(DrawType::TRIANGLE_STRIP,
                  std::vector<Vec3f>{
                      Vec3f{0.0f, 0.0f, 1.0f},
                      Vec3f{0.0f, 0.25f + height, 1.0f},
                      Vec3f{0.2f + width, 0.0f, 1.0f},
                      Vec3f{0.2f + width, 0.25f + height, 1.0f},
                  });
    m_opengl.apply(XDisable{XOption::BLEND});

    // text
    m_opengl.glTranslatef(-x / 2.0f, -y / 2.0f, 0.0f);
    renderText(x + 0.1f, y + 0.3f, name);
    m_opengl.apply(XEnable{XOption::DEPTH_TEST});

    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawFlow(const Room *const room,
                                   const RoomIndex &rooms,
                                   const ExitDirection exitDirection)
{
    // Start drawing
    m_opengl.glPushMatrix();

    // Prepare pen
    QColor color = QColor(76, 216, 255);
    m_opengl.apply(XColor4f{color});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDevicePointSize{4.0});
    m_opengl.apply(XDeviceLineWidth{1.0f});

    // Draw part in this room
    if (room->getPosition().z == m_currentLayer) {
        m_opengl.callList(m_gllist.flow.begin[exitDirection]);
    }

    // Draw part in adjacent room
    const ExitDirection targetDir = opposite(exitDirection);
    const ExitsList &exitslist = room->getExitsList();
    const Exit &sourceExit = exitslist[exitDirection];

    // For each outgoing connections
    for (auto targetId : sourceExit.outRange()) {
        const Room *const targetRoom = rooms[targetId];
        const auto &pos = targetRoom->getPosition();
        if (pos.z == m_currentLayer) {
            m_opengl.setMatrix(MatrixType::MODELVIEW, getTranslationMatrix(pos.x, pos.y));
            m_opengl.callList(m_gllist.flow.end[targetDir]);
        }
    }

    // Finish pen
    m_opengl.apply(XDeviceLineWidth{2.0});
    m_opengl.apply(XDevicePointSize{2.0});
    m_opengl.apply(XDisable{XOption::BLEND});

    // Terminate drawing
    color = Qt::black;
    m_opengl.apply(XColor4f{color});
    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawExit(const Room *const room,
                                   const RoomIndex &rooms,
                                   const qint32 layer,
                                   const ExitDirection dir)
{
    const auto drawNotMappedExits = getConfig().canvas.drawNotMappedExits;

    const auto wallList = m_gllist.wall[dir];
    const auto doorList = m_gllist.door[dir];
    assert(wallList.isValid());
    assert(doorList.isValid());

    const Exit &exit = room->exit(dir);
    const ExitFlags flags = exit.getExitFlags();
    const auto isExit = flags.isExit();
    const auto isDoor = flags.isDoor();

    if (isExit && drawNotMappedExits && exit.outIsEmpty()) { // zero outgoing connections
        drawListWithLineStipple(wallList, WALL_COLOR_NOTMAPPED);
    } else {
        const auto color = getWallColor(flags);
        if (color != Qt::transparent)
            drawListWithLineStipple(wallList, color);

        if (flags.isFlow()) {
            drawFlow(room, rooms, dir);
        }
    }

    // wall
    if (!isExit || isDoor) {
        if (!isDoor && !exit.outIsEmpty()) {
            drawListWithLineStipple(wallList, WALL_COLOR_WALL_DOOR);
        } else {
            m_opengl.apply(getWallExitColor(layer));
            m_opengl.callList(wallList);
        }
    }
    // door
    if (isDoor) {
        m_opengl.apply(getWallExitColor(layer));
        m_opengl.callList(doorList);
    }
}

void MapCanvasRoomDrawer::drawRooms(const LayerToRooms &layerToRooms,
                                    const RoomIndex &roomIndex,
                                    const RoomLocks &locks)
{
    const auto wantExtraDetail = m_scaleFactor * m_mapCanvasData.m_currentStepScaleFactor >= 0.15f;
    for (const auto &layer : layerToRooms) {
        const auto &rooms = layer.second;
        for (const auto &room : rooms) {
            drawRoom(room, wantExtraDetail);
        }
        for (const auto &room : rooms) {
            drawWallsAndExits(room, roomIndex);
        }
        for (const auto &room : rooms) {
            drawBoost(room, locks);
        }
    }
    // Lines (Connections, InfoMark Lines)
    if (wantExtraDetail) {
        for (const auto &layer : layerToRooms) {
            for (const auto &room : layer.second) {
                drawRoomConnectionsAndDoors(room, roomIndex);
            }
        }
    }
}

void MapCanvasRoomDrawer::drawRoom(const Room *const room, bool wantExtraDetail)
{
    const auto x = room->getPosition().x;
    const auto y = room->getPosition().y;
    const auto z = room->getPosition().z;
    const auto layer = z - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.glTranslatef(static_cast<float>(x) - 0.5f,
                          static_cast<float>(y) - 0.5f,
                          ROOM_Z_DISTANCE * static_cast<float>(layer));

    // TODO(nschimme): https://stackoverflow.com/questions/6017176/gllinestipple-deprecated-in-opengl-3-1
    m_opengl.apply(LineStippleType::TWO);

    const auto roomColor = getRoomColor(layer);

    // Make dark and troll safe rooms look dark
    const bool isDark = room->getLightType() == RoomLightType::DARK;
    const bool hasNoSundeath = room->getSundeathType() == RoomSundeathType::NO_SUNDEATH;
    m_opengl.apply((isDark || hasNoSundeath) ? getRoomColor(layer, isDark, hasNoSundeath)
                                             : roomColor);

    if (layer > 0) {
        if (!getConfig().canvas.drawUpperLayersTextured) {
            m_opengl.apply(XEnable{XOption::BLEND});
            m_opengl.callList(m_gllist.room);
            m_opengl.apply(XDisable{XOption::BLEND});
            m_opengl.glPopMatrix();
            return;
        }
        m_opengl.apply(XEnable{XOption::POLYGON_STIPPLE});
    }

    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XEnable{XOption::TEXTURE_2D});

    const auto roomTerrainType = room->getTerrainType();
    const RoadIndex roadIndex = getRoadIndex(*room);
    QOpenGLTexture *const texture = (roomTerrainType == RoomTerrainType::ROAD)
                                        ? m_textures.road[roadIndex].get()
                                        : m_textures.terrain[roomTerrainType].get();
    texture->bind();
    m_opengl.callList(m_gllist.room);

    m_opengl.apply(XDisable{XOption::TEXTURE_2D});

    // REVISIT: Turn this into a texture or move it into a different rendering stage
    // Draw a little dark red cross on noride rooms
    if (room->getRidableType() == RoomRidableType::NOT_RIDABLE) {
        m_opengl.glTranslatef(0, 0, ROOM_Z_LAYER_BUMP);
        m_opengl.apply(XColor4f{0.5f, 0.0f, 0.0f, 0.9f});
        m_opengl.apply(XDeviceLineWidth{3.0});
        m_opengl.draw(DrawType::LINES,
                      std::vector<Vec3f>{
                          Vec3f{0.6f, 0.2f, 0.0f},
                          Vec3f{0.8f, 0.4f, 0.0f},
                          Vec3f{0.8f, 0.2f, 0.0f},
                          Vec3f{0.6f, 0.4f, 0.0f},
                      });
    }

    // Only display at a certain scale
    if (wantExtraDetail) {
        // Restore room color from dark room or noride red cross
        m_opengl.apply(XEnable{XOption::TEXTURE_2D});
        m_opengl.apply(roomColor);

        const RoomMobFlags mf = room->getMobFlags();
        const RoomLoadFlags lf = room->getLoadFlags();

        // Trail Support
        if (roadIndex != RoadIndex::NONE && roomTerrainType != RoomTerrainType::ROAD) {
            m_opengl.glTranslatef(0, 0, ROOM_Z_LAYER_BUMP);
            alphaOverlayTexture(m_textures.trail[roadIndex].get());
        }

        for (const RoomMobFlag flag : ALL_MOB_FLAGS) {
            if (mf.contains(flag)) {
                m_opengl.glTranslatef(0, 0, ROOM_Z_LAYER_BUMP);
                alphaOverlayTexture(m_textures.mob[flag].get());
            }
        }

        for (const RoomLoadFlag flag : ALL_LOAD_FLAGS) {
            if (lf.contains(flag)) {
                m_opengl.glTranslatef(0, 0, ROOM_Z_LAYER_BUMP);
                alphaOverlayTexture(m_textures.load[flag].get());
            }
        }

        if (getConfig().canvas.showUpdated && !room->isUpToDate()) {
            m_opengl.glTranslatef(0, 0, ROOM_Z_LAYER_BUMP);
            alphaOverlayTexture(m_textures.update.get());
        }
        m_opengl.apply(XDisable{XOption::BLEND});

        m_opengl.apply(XDisable{XOption::TEXTURE_2D});
    }

    if (layer > 0)
        m_opengl.apply(XDisable{XOption::POLYGON_STIPPLE});

    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawWallsAndExits(const Room *room, const RoomIndex &rooms)
{
    const auto x = room->getPosition().x;
    const auto y = room->getPosition().y;
    const auto layer = room->getPosition().z - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.glTranslatef(static_cast<float>(x) - 0.5f,
                          static_cast<float>(y) - 0.5f,
                          ROOM_Z_DISTANCE * static_cast<float>(layer));

    // walls
    m_opengl.glTranslatef(0, 0, ROOM_WALLS_BUMP);

    if (layer > 0)
        m_opengl.apply(XEnable{XOption::BLEND});

    m_opengl.apply(XDevicePointSize{3.0});
    m_opengl.apply(XDeviceLineWidth{2.4f});

    for (auto dir : ALL_EXITS_NESW) {
        drawExit(room, rooms, layer, dir);
    }

    m_opengl.apply(XDevicePointSize{3.0});
    m_opengl.apply(XDeviceLineWidth{2.0});

    for (auto dir : {ExitDirection ::UP, ExitDirection::DOWN}) {
        const auto &exitList = m_gllist.exit;
        const auto &updown = dir == ExitDirection::UP ? exitList.up : exitList.down;
        drawVertical(room, rooms, layer, dir, updown, m_gllist.door[dir]);
    }

    if (layer > 0)
        m_opengl.apply(XDisable{XOption::BLEND});

    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawBoost(const Room *const room, const RoomLocks &locks)
{
    const auto x = room->getPosition().x;
    const auto y = room->getPosition().y;
    const auto layer = room->getPosition().z - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.glTranslatef(static_cast<float>(x) - 0.5f,
                          static_cast<float>(y) - 0.5f,
                          ROOM_Z_DISTANCE * static_cast<float>(layer));

    // Boost the colors of rooms that are on a different layer
    m_opengl.glTranslatef(0, 0, ROOM_BOOST_BUMP);
    if (layer < 0) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XColor4f{Qt::black, 0.5f - 0.03f * static_cast<float>(layer)});
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XDisable{XOption::BLEND});
    } else if (layer > 0) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XColor4f{Qt::white, 0.1f});
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XDisable{XOption::BLEND});
    }
    // Locked rooms have a red hint
    if (!locks[room->getId()].empty()) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XColor4f{0.6f, 0.0f, 0.0f, 0.2f});
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XDisable{XOption::BLEND});
    }

    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawRoomConnectionsAndDoors(const Room *const room, const RoomIndex &rooms)
{
    /* If a room isn't fake, then we know its ExitsList will have NUM_EXITS elements */
    if (deref(room).isFake()) {
        qWarning() << "Fake room? How did that happen?";
        return;
    }

    auto sourceId = room->getId();
    const Room *targetRoom = nullptr;
    const ExitsList &exitslist = room->getExitsList();
    bool oneway = false;
    float rx = 0;
    float ry = 0;

    const auto wantDoorNames = getConfig().canvas.drawDoorNames
                               && (m_mapCanvasData.m_scaleFactor
                                       * m_mapCanvasData.m_currentStepScaleFactor
                                   >= 0.40f);
    for (const auto i : ALL_EXITS7) {
        const auto opp = opposite(i);
        ExitDirection targetDir = opp;
        const Exit &sourceExit = exitslist[i];
        // outgoing connections
        for (const auto &outTargetId : sourceExit.outRange()) {
            targetRoom = rooms[outTargetId];
            if (!targetRoom) {
                qWarning() << "Source room" << sourceId.asUint32() << "has target room"
                           << outTargetId.asUint32() << "which does not exist!";
                continue;
            }
            rx = static_cast<float>(targetRoom->getPosition().x);
            ry = static_cast<float>(targetRoom->getPosition().y);
            if ((outTargetId >= sourceId) || // draw exits if outTargetId >= sourceId ...
                                             // or if target room is not visible
                ((rx < m_visible1.x - 1.0f) || (rx > m_visible2.x + 1.0f))
                || ((ry < m_visible1.y - 1.0f) || (ry > m_visible2.y + 1.0f))) {
                if (targetRoom->exit(targetDir).containsOut(sourceId)) {
                    oneway = false;
                } else {
                    oneway = true;
                    for (const auto j : ALL_EXITS7) {
                        if (targetRoom->exit(j).containsOut(sourceId)) {
                            targetDir = j;
                            oneway = false;
                            break;
                        }
                    }
                }
                if (oneway) {
                    drawConnection(room, targetRoom, i, targetDir, true, room->exit(i).isExit());
                } else {
                    drawConnection(room,
                                   targetRoom,
                                   i,
                                   targetDir,
                                   false,
                                   room->exit(i).isExit() && targetRoom->exit(targetDir).isExit());
                }
            } else if (!sourceExit.containsIn(outTargetId)) { // ... or if they are outgoing oneways
                oneway = true;
                for (const auto j : ALL_EXITS7) {
                    if (targetRoom->exit(j).containsOut(sourceId)) {
                        targetDir = j;
                        oneway = false;
                        break;
                    }
                }
                if (oneway) {
                    drawConnection(room, targetRoom, i, opp, true, sourceExit.isExit());
                }
            }

            // incoming connections (only for oneway connections from rooms, that are not visible)
            for (const auto &inTargetId : exitslist[i].inRange()) {
                targetRoom = rooms[inTargetId];
                rx = targetRoom->getPosition().x;
                ry = targetRoom->getPosition().y;

                if (((rx < m_visible1.x - 1.0f) || (rx > m_visible2.x + 1.0f))
                    || ((ry < m_visible1.y - 1.0f) || (ry > m_visible2.y + 1.0f))) {
                    if (!targetRoom->exit(opp).containsIn(sourceId)) {
                        drawConnection(targetRoom,
                                       room,
                                       opp,
                                       i,
                                       true,
                                       targetRoom->exit(opp).isExit());
                    }
                }
            }

            // draw door names
            if (wantDoorNames && room->exit(i).isDoor() && room->exit(i).hasDoorName()
                && room->exit(i).isHiddenExit()) {
                if (targetRoom->exit(opp).containsOut(sourceId)) {
                    targetDir = opp;
                } else {
                    for (const auto j : ALL_EXITS7) {
                        if (targetRoom->exit(j).containsOut(sourceId)) {
                            targetDir = j;
                            break;
                        }
                    }
                }
                drawRoomDoorName(room, i, targetRoom, targetDir);
            }
        }
    }
}

void MapCanvasRoomDrawer::drawVertical(
    const Room *const room,
    const RoomIndex &rooms,
    const qint32 layer,
    const ExitDirection direction,
    const MapCanvasData::DrawLists::ExitUpDown::OpaqueTransparent &exlists,
    const XDisplayList doorlist)
{
    if (!isUpDown(direction))
        throw std::invalid_argument("dir");

    const auto transparent = exlists.transparent;
    const auto opaque = exlists.opaque;

    const Exit &roomExit = room->exit(direction);
    const auto flags = roomExit.getExitFlags();
    if (!flags.isExit())
        return;

    const auto drawNotMappedExits = getConfig().canvas.drawNotMappedExits;
    if (drawNotMappedExits && roomExit.outIsEmpty()) { // zero outgoing connections
        drawListWithLineStipple(transparent, WALL_COLOR_NOTMAPPED);
        return;
    }

    const auto color = getVerticalColor(flags, m_mapCanvasData.g_noFleeColor);
    if (color != Qt::transparent)
        drawListWithLineStipple(transparent, color);

    /* NOTE: semi-bugfix: The opaque display list modifies color to black,
     * but the transparent display list doesn't.
     * Door display list doesn't set its own color, but flow does. */
    const auto useTransparent = layer > 0;
    m_opengl.apply(getWallExitColor(layer), useTransparent ? transparent : opaque);

    if (flags.isDoor()) {
        m_opengl.callList(doorlist);
    }

    if (flags.isFlow()) {
        drawFlow(room, rooms, direction);
    }
}

void MapCanvasRoomDrawer::drawListWithLineStipple(const XDisplayList list, const QColor &color)
{
    if (color.alphaF() != 1.0)
        qWarning() << __FUNCTION__ << color;

    m_opengl.apply(XEnable{XOption::LINE_STIPPLE});
    m_opengl.apply(XColor4f{color});
    m_opengl.callList(list);
    m_opengl.apply(XDisable{XOption::LINE_STIPPLE});
}

void MapCanvasRoomDrawer::drawConnection(const Room *leftRoom,
                                         const Room *rightRoom,
                                         ExitDirection startDir,
                                         ExitDirection endDir,
                                         bool oneway,
                                         bool inExitFlags)
{
    assert(leftRoom != nullptr);
    assert(rightRoom != nullptr);

    const qint32 leftX = leftRoom->getPosition().x;
    const qint32 leftY = leftRoom->getPosition().y;
    const qint32 leftZ = leftRoom->getPosition().z;
    const qint32 rightX = rightRoom->getPosition().x;
    const qint32 rightY = rightRoom->getPosition().y;
    const qint32 rightZ = rightRoom->getPosition().z;
    const qint32 dX = rightX - leftX;
    const qint32 dY = rightY - leftY;
    const qint32 dZ = rightZ - leftZ;

    const qint32 leftLayer = leftZ - m_currentLayer;
    const qint32 rightLayer = rightZ - m_currentLayer;

    if ((rightZ != m_currentLayer) && (leftZ != m_currentLayer)) {
        return;
    }

    bool neighbours = false;

    if ((dX == 0) && (dY == -1) && (dZ == 0)) {
        if ((startDir == ExitDirection::NORTH) && (endDir == ExitDirection::SOUTH) && !oneway) {
            return;
        }
        neighbours = true;
    }
    if ((dX == 0) && (dY == +1) && (dZ == 0)) {
        if ((startDir == ExitDirection::SOUTH) && (endDir == ExitDirection::NORTH) && !oneway) {
            return;
        }
        neighbours = true;
    }
    if ((dX == +1) && (dY == 0) && (dZ == 0)) {
        if ((startDir == ExitDirection::EAST) && (endDir == ExitDirection::WEST) && !oneway) {
            return;
        }
        neighbours = true;
    }
    if ((dX == -1) && (dY == 0) && (dZ == 0)) {
        if ((startDir == ExitDirection::WEST) && (endDir == ExitDirection::EAST) && !oneway) {
            return;
        }
        neighbours = true;
    }

    m_opengl.glPushMatrix();
    m_opengl.glTranslatef(leftX - 0.5f, leftY - 0.5f, 0.0f);

    m_opengl.apply(XColor4f{inExitFlags ? Qt::white : Qt::red, 0.70f});

    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDevicePointSize{2.0});
    m_opengl.apply(XDeviceLineWidth{2.0});

    const float srcZ = ROOM_Z_DISTANCE * static_cast<float>(leftLayer) + 0.3f;
    const float dstZ = ROOM_Z_DISTANCE * static_cast<float>(rightLayer) + 0.3f;

    drawConnectionLine(startDir, endDir, oneway, neighbours, dX, dY, srcZ, dstZ);
    drawConnectionTriangles(startDir, endDir, oneway, dX, dY, srcZ, dstZ);

    m_opengl.apply(XDisable{XOption::BLEND});
    m_opengl.apply(XColor4f{Qt::white, 0.70f});
    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawConnectionTriangles(const ExitDirection startDir,
                                                  const ExitDirection endDir,
                                                  const bool oneway,
                                                  const qint32 dX,
                                                  const qint32 dY,
                                                  const float srcZ,
                                                  const float dstZ)
{
    if (oneway) {
        drawConnEndTri1Way(endDir, dX, dY, dstZ);
    } else {
        drawConnStartTri(startDir, srcZ);
        drawConnEndTri(endDir, dX, dY, dstZ);
    }
}

void MapCanvasRoomDrawer::drawConnectionLine(const ExitDirection startDir,
                                             const ExitDirection endDir,
                                             const bool oneway,
                                             const bool neighbours,
                                             const qint32 dX,
                                             const qint32 dY,
                                             const float srcZ,
                                             const float dstZ)
{
    std::vector<Vec3f> points{};
    ConnectionLineBuilder lb{points};
    lb.drawConnLineStart(startDir, neighbours, srcZ);
    if (points.empty())
        return;

    if (oneway)
        lb.drawConnLineEnd1Way(endDir, dX, dY, dstZ);
    else
        lb.drawConnLineEnd2Way(endDir, neighbours, dX, dY, dstZ);

    if (points.empty())
        return;

    drawLineStrip(points);
}

void MapCanvasRoomDrawer::drawLineStrip(const std::vector<Vec3f> &points)
{
    m_opengl.draw(DrawType::LINE_STRIP, points);
}

void MapCanvasRoomDrawer::drawConnStartTri(const ExitDirection startDir, const float srcZ)
{
    switch (startDir) {
    case ExitDirection::NORTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{0.68f, +0.1f, srcZ},
                          Vec3f{0.82f, +0.1f, srcZ},
                          Vec3f{0.75f, +0.3f, srcZ},
                      });
        break;
    case ExitDirection::SOUTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{0.18f, 0.9f, srcZ},
                          Vec3f{0.32f, 0.9f, srcZ},
                          Vec3f{0.25f, 0.7f, srcZ},
                      });
        break;
    case ExitDirection::EAST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{0.9f, 0.18f, srcZ},
                          Vec3f{0.9f, 0.32f, srcZ},
                          Vec3f{0.7f, 0.25f, srcZ},
                      });
        break;
    case ExitDirection::WEST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{0.1f, 0.68f, srcZ},
                          Vec3f{0.1f, 0.82f, srcZ},
                          Vec3f{0.3f, 0.75f, srcZ},
                      });
        break;

    case ExitDirection::UP:
    case ExitDirection::DOWN:
    case ExitDirection::UNKNOWN:
    case ExitDirection::NONE:
        break;
    }
}

void MapCanvasRoomDrawer::drawConnEndTri(const ExitDirection endDir,
                                         const qint32 dX,
                                         const qint32 dY,
                                         const float dstZ)
{
    switch (endDir) {
    case ExitDirection::NORTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.68f, dY + 0.1f, dstZ},
                          Vec3f{dX + 0.82f, dY + 0.1f, dstZ},
                          Vec3f{dX + 0.75f, dY + 0.3f, dstZ},
                      });
        break;
    case ExitDirection::SOUTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.18f, dY + 0.9f, dstZ},
                          Vec3f{dX + 0.32f, dY + 0.9f, dstZ},
                          Vec3f{dX + 0.25f, dY + 0.7f, dstZ},
                      });
        break;
    case ExitDirection::EAST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.9f, dY + 0.18f, dstZ},
                          Vec3f{dX + 0.9f, dY + 0.32f, dstZ},
                          Vec3f{dX + 0.7f, dY + 0.25f, dstZ},
                      });
        break;
    case ExitDirection::WEST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.1f, dY + 0.68f, dstZ},
                          Vec3f{dX + 0.1f, dY + 0.82f, dstZ},
                          Vec3f{dX + 0.3f, dY + 0.75f, dstZ},
                      });
        break;

    case ExitDirection::UP:
    case ExitDirection::DOWN:
        // Do not draw triangles for 2-way up/down
        break;
    case ExitDirection::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUpDownUnknown(dX, dY, dstZ);
        break;
    case ExitDirection::NONE:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriNone(dX, dY, dstZ);
        break;
    }
}
void MapCanvasRoomDrawer::drawConnEndTri1Way(const ExitDirection endDir,
                                             const qint32 dX,
                                             const qint32 dY,
                                             const float dstZ)
{
    switch (endDir) {
    case ExitDirection::NORTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.18f, dY + 0.1f, dstZ},
                          Vec3f{dX + 0.32f, dY + 0.1f, dstZ},
                          Vec3f{dX + 0.25f, dY + 0.3f, dstZ},
                      });
        break;
    case ExitDirection::SOUTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.68f, dY + 0.9f, dstZ},
                          Vec3f{dX + 0.82f, dY + 0.9f, dstZ},
                          Vec3f{dX + 0.75f, dY + 0.7f, dstZ},
                      });
        break;
    case ExitDirection::EAST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.9f, dY + 0.68f, dstZ},
                          Vec3f{dX + 0.9f, dY + 0.82f, dstZ},
                          Vec3f{dX + 0.7f, dY + 0.75f, dstZ},
                      });
        break;
    case ExitDirection::WEST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3f>{
                          Vec3f{dX + 0.1f, dY + 0.18f, dstZ},
                          Vec3f{dX + 0.1f, dY + 0.32f, dstZ},
                          Vec3f{dX + 0.3f, dY + 0.25f, dstZ},
                      });
        break;

    case ExitDirection::UP:
    case ExitDirection::DOWN:
    case ExitDirection::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUpDownUnknown(dX, dY, dstZ);
        break;
    case ExitDirection::NONE:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriNone(dX, dY, dstZ);
        break;
    }
}

void MapCanvasRoomDrawer::drawConnEndTriNone(qint32 dX, qint32 dY, float dstZ)
{
    m_opengl.draw(DrawType::TRIANGLES,
                  std::vector<Vec3f>{
                      Vec3f{dX + 0.5f, dY + 0.5f, dstZ},
                      Vec3f{dX + 0.7f, dY + 0.55f, dstZ},
                      Vec3f{dX + 0.55f, dY + 0.7f, dstZ},
                  });
}

void MapCanvasRoomDrawer::drawConnEndTriUpDownUnknown(qint32 dX, qint32 dY, float dstZ)
{
    m_opengl.draw(DrawType::TRIANGLES,
                  std::vector<Vec3f>{
                      Vec3f{dX + 0.5f, dY + 0.5f, dstZ},
                      Vec3f{dX + 0.7f, dY + 0.55f, dstZ},
                      Vec3f{dX + 0.55f, dY + 0.7f, dstZ},
                  });
}

void MapCanvasRoomDrawer::renderText(const float x,
                                     const float y,
                                     const QString &text,
                                     const QColor &color,
                                     const FontFormatFlags fontFormatFlag,
                                     const float rotationAngle)
{
    // http://stackoverflow.com/questions/28216001/how-to-render-text-with-qopenglwidget/28517897
    const QVector3D projected = m_mapCanvasData.project(QVector3D{x, y, CAMERA_Z_DISTANCE});
    const auto textPosX = projected.x();
    const auto textPosY = static_cast<float>(m_mapCanvasData.height())
                          - projected.y(); // y is inverted
    m_opengl.renderTextAt(textPosX, textPosY, text, color, fontFormatFlag, rotationAngle);
}
