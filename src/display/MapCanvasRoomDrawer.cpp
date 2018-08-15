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
#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
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

class RAIIColorSaver;

/*
 * NOTE: glGetXXX() is a huge performance killer because it requires a sync with the driver.
 * But please don't blindly take this comment to mean "add a color variable to the class."
 * The better solution is to always specify what color we want for each draw call!
 *
 * Eventually we'll want to make a LineBatch and TriangleBatch objects that buffer up
 * as many lines / triangles as possible before calling a custom shader that applies the
 * specified colors / styles, using only a single eventual draw call per buffer.
 * Once we have that, we won't need to bother setting the legacy OpenGL 1.x color property.
 */
#define SAVE_COLOR() \
    RAIIColorSaver saved_color { this->m_opengl }
class RAIIColorSaver final
{
private:
    OpenGL &m_opengl;
    std::array<GLdouble, 4> oldcolour{};

public:
    RAIIColorSaver(RAIIColorSaver &&) = delete;
    RAIIColorSaver(const RAIIColorSaver &) = delete;
    RAIIColorSaver &operator=(RAIIColorSaver &&) = delete;
    RAIIColorSaver &operator=(const RAIIColorSaver &) = delete;

public:
    explicit RAIIColorSaver(OpenGL &opengl)
        : m_opengl{opengl}
    {
        m_opengl.glGetDoublev(GL_CURRENT_COLOR, oldcolour.data());
    }

public:
    ~RAIIColorSaver() noexcept(false) /* allow exceptions */
    {
        m_opengl.apply(XColor4d{oldcolour[0], oldcolour[1], oldcolour[2], oldcolour[3]});
    }
};

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

QMatrix4x4 getTranslationMatrix(float x, float y)
{
    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(x, y, 0);
    return model;
}
QMatrix4x4 getTranslationMatrix(const int x, const int y)
{
    return getTranslationMatrix(static_cast<float>(x), static_cast<float>(y));
}

QColor getInforMarkColor(InfoMarkClass infoMarkClass)
{
    switch (infoMarkClass) {
    case InfoMarkClass::HERB:
        return QColor(0, 200, 0); // Dark green
    case InfoMarkClass::RIVER:
        return QColor(76, 216, 255); // Cyan-like
    case InfoMarkClass::MOB:
        return QColor(177, 27, 27); // Dark red
    case InfoMarkClass::COMMENT:
        return Qt::lightGray;
    case InfoMarkClass::ROAD:
        return QColor(140, 83, 58); // Maroonish
    case InfoMarkClass::OBJECT:
        return Qt::yellow;

    case InfoMarkClass::GENERIC:
    case InfoMarkClass::PLACE:
    case InfoMarkClass::ACTION:
    case InfoMarkClass::LOCALITY:
        return Qt::white;
    }

    return Qt::white; // Default color;
}

FontFormatFlags getFontFormatFlags(InfoMarkClass infoMarkClass)
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
    const auto drawNoMatchExits = Config().canvas.drawNoMatchExits;

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

static QString getDoorPostFix(const Room *const room, const ExitDirection dir)
{
    static constexpr const auto SHOWN_FLAGS = DoorFlag::HIDDEN | DoorFlag::NEED_KEY
                                              | DoorFlag::NO_PICK | DoorFlag::DELAYED;

    const DoorFlags flags = room->exit(dir).getDoorFlags();
    if (!flags.containsAny(SHOWN_FLAGS))
        return QString{};

    return QString::asprintf(" [%s%s%s%s]",
                             flags.isHidden() ? "h" : "",
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
    MarkerListIterator m(m_data->getMarkersList());
    while (m.hasNext()) {
        drawInfoMark(m.next());
    }
}

void MapCanvasRoomDrawer::drawInfoMark(InfoMark *marker)
{
    const qreal x1 = static_cast<double>(marker->getPosition1().x) / 100.0;
    const qreal y1 = static_cast<double>(marker->getPosition1().y) / 100.0;
    const int layer = marker->getPosition1().z;
    qreal x2 = static_cast<double>(marker->getPosition2().x) / 100.0;
    qreal y2 = static_cast<double>(marker->getPosition2().y) / 100.0;
    const qreal dx = x2 - x1;
    const qreal dy = y2 - y1;

    qreal width = 0.0;
    qreal height = 0.0;

    if (layer != m_currentLayer) {
        return;
    }

    const auto infoMarkType = marker->getType();
    const auto infoMarkClass = marker->getClass();

    // Color depends of the class of the InfoMark
    const QColor color = getInforMarkColor(infoMarkClass);
    const auto fontFormatFlag = getFontFormatFlags(infoMarkClass);

    if (infoMarkType == InfoMarkType::TEXT) {
        width = getScaledFontWidth(marker->getText());
        height = getScaledFontHeight();
        x2 = x1 + width;
        y2 = y1 + height;
    }

    //check if marker is visible
    if (((x1 + 1.0 < static_cast<double>(m_visible1.x))
         || (x1 - 1.0 > static_cast<double>(m_visible2.x) + 1.0))
        && ((x2 + 1.0 < static_cast<double>(m_visible1.x))
            || (x2 - 1.0 > static_cast<double>(m_visible2.x) + 1))) {
        return;
    }
    if (((y1 + 1.0 < static_cast<double>(m_visible1.y))
         || (y1 - 1.0 > static_cast<double>(m_visible2.y) + 1.0))
        && ((y2 + 1.0 < static_cast<double>(m_visible1.y))
            || (y2 - 1.0 > static_cast<double>(m_visible2.y) + 1.0))) {
        return;
    }

    m_opengl.glPushMatrix();
    m_opengl.glTranslated(x1 /*-0.5*/, y1 /*-0.5*/, 0.0);

    switch (infoMarkType) {
    case InfoMarkType::TEXT:
        // Render background
        m_opengl.apply(XColor4d{0, 0, 0, 0.3});
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.draw(DrawType::POLYGON,
                      std::vector<Vec3d>{
                          Vec3d{0.0, 0.0, 1.0},
                          Vec3d{0.0, 0.25 + height, 1.0},
                          Vec3d{0.2 + width, 0.25 + height, 1.0},
                          Vec3d{0.2 + width, 0.0, 1.0},
                      });
        m_opengl.apply(XDisable{XOption::BLEND});

        // Render text proper
        m_opengl.glTranslated(-x1 / 2.0, -y1 / 2.0, 0.0);
        renderText(static_cast<float>(x1 + 0.1),
                   static_cast<float>(y1 + 0.25),
                   marker->getText(),
                   color,
                   fontFormatFlag,
                   marker->getRotationAngle());
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        break;
    case InfoMarkType::LINE:
        m_opengl.apply(XColor4d{color, 0.70});
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XDevicePointSize{2.0});
        m_opengl.apply(XDeviceLineWidth{2.0});
        m_opengl.draw(DrawType::LINES,
                      std::vector<Vec3d>{
                          Vec3d{0.0, 0.0, 0.1},
                          Vec3d{dx, dy, 0.1},
                      });
        m_opengl.apply(XDisable{XOption::BLEND});
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        break;
    case InfoMarkType::ARROW:
        m_opengl.apply(XColor4d{color, 0.70});
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XDevicePointSize{2.0});
        m_opengl.apply(XDeviceLineWidth{2.0});
        m_opengl.draw(DrawType::LINE_STRIP,
                      std::vector<Vec3d>{Vec3d{0.0, 0.05, 1.0},
                                         Vec3d{dx - 0.2, dy + 0.1, 1.0},
                                         Vec3d{dx - 0.1, dy + 0.1, 1.0}});
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{Vec3d{dx - 0.1, dy + 0.1 - 0.07, 1.0},
                                         Vec3d{dx - 0.1, dy + 0.1 + 0.07, 1.0},
                                         Vec3d{dx + 0.1, dy + 0.1, 1.0}});
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

    const auto srcX = sourceRoom->getPosition().x;
    const auto srcY = sourceRoom->getPosition().y;
    const auto srcZ = sourceRoom->getPosition().z;
    const auto tarX = targetRoom->getPosition().x;
    const auto tarY = targetRoom->getPosition().y;
    const auto tarZ = targetRoom->getPosition().z;
    const auto dX = srcX - tarX;
    const auto dY = srcY - tarY;

    if (srcZ != m_currentLayer && tarZ != m_currentLayer) {
        return;
    }

    QString name;
    bool together = false;

    if (targetRoom->exit(targetDir).isDoor()
        // the other room has a door?
        && targetRoom->exit(targetDir).hasDoorName()
        // has a door on both sides...
        && abs(dX) <= 1 && abs(dY) <= 1) { // the door is close by!
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
        name = sourceRoom->exit(sourceDir).getDoorName();
    }

    const auto width = getScaledFontWidth(name);
    const auto height = getScaledFontHeight();

    auto boxX = 0, boxY = 0;
    if (together) {
        boxX = srcX - (width / 2.0f) - (dX * 0.5f);
        boxY = srcY - 0.5f - (dY * 0.5f);
    } else {
        boxX = srcX - (width / 2.0f);
        switch (sourceDir) {
        case ExitDirection::NORTH:
            boxY = srcY - 0.65;
            break;
        case ExitDirection::SOUTH:
            boxY = srcY - 0.15;
            break;
        case ExitDirection::WEST:
            boxY = srcY - 0.5;
            break;
        case ExitDirection::EAST:
            boxY = srcY - 0.35;
            break;
        case ExitDirection::UP:
            boxY = srcY - 0.85;
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
    const QString &name, const qreal x, const qreal y, const qreal width, const qreal height)
{
    const qreal boxX2 = x + width;
    const qreal boxY2 = y + height;

    //check if box is visible
    if (((x + 1.0 < static_cast<double>(m_visible1.x))
         || (x - 1 > static_cast<double>(m_visible2.x) + 1))
        && ((boxX2 + 1 < static_cast<double>(m_visible1.x))
            || (boxX2 - 1.0 > static_cast<double>(m_visible2.x) + 1.0))) {
        return;
    }
    if (((y + 1.0 < static_cast<double>(m_visible1.y))
         || (y - 1 > static_cast<double>(m_visible2.y) + 1))
        && ((boxY2 + 1.0 < static_cast<double>(m_visible1.y))
            || (boxY2 - 1.0 > static_cast<double>(m_visible2.y) + 1.0))) {
        return;
    }

    m_opengl.glPushMatrix();

    m_opengl.glTranslated(x /*-0.5*/, y /*-0.5*/, 0);

    // Render background
    m_opengl.apply(XColor4d{0, 0, 0, 0.3});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.draw(DrawType::POLYGON,
                  std::vector<Vec3d>{
                      Vec3d{0.0, 0.0, 1.0},
                      Vec3d{0.0, 0.25 + height, 1.0},
                      Vec3d{0.2 + width, 0.25 + height, 1.0},
                      Vec3d{0.2 + width, 0.0, 1.0},
                  });
    m_opengl.apply(XDisable{XOption::BLEND});

    // text
    m_opengl.glTranslated(-x / 2.0, -y / 2.0, 0.0);
    renderText(static_cast<float>(x + 0.1), static_cast<float>(y + 0.25), name);
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
    m_opengl.apply(XColor4d{color});
    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDevicePointSize{4.0});
    m_opengl.apply(XDeviceLineWidth{1.0});

    // Draw part in this room
    if (room->getPosition().z == m_currentLayer) {
        m_opengl.callList(m_gllist.flow.begin[exitDirection]);
    }

    // Draw part in adjacent room
    const ExitDirection targetDir = opposite(exitDirection);
    const ExitsList &exitslist = room->getExitsList();
    const Exit &sourceExit = exitslist[exitDirection];

    //For each outgoing connections
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
    m_opengl.apply(XColor4d{color});
    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawExit(const Room *const room,
                                   const RoomIndex &rooms,
                                   const ExitDirection dir)
{
    const auto drawNotMappedExits = Config().canvas.drawNotMappedExits;

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

    //wall
    if (!isExit || isDoor) {
        if (!isDoor && !exit.outIsEmpty()) {
            drawListWithLineStipple(wallList, WALL_COLOR_WALL_DOOR);
        } else {
            m_opengl.callList(wallList);
        }
    }
    //door
    if (isDoor) {
        m_opengl.callList(doorList);
    }
}

void MapCanvasRoomDrawer::drawRoom(const Room *const room,
                                   const RoomIndex &rooms,
                                   const RoomLocks &locks)
{
    if (room == nullptr) {
        return;
    }

    const auto wantExtraDetail = m_scaleFactor >= 0.15f;
    const auto x = room->getPosition().x;
    const auto y = room->getPosition().y;
    const auto z = room->getPosition().z;
    const auto layer = z - m_currentLayer;

    m_opengl.glPushMatrix();
    m_opengl.glTranslated(static_cast<double>(x) - 0.5,
                          static_cast<double>(y) - 0.5,
                          ROOM_Z_DISTANCE * static_cast<double>(layer));

    // TODO(nschimme): https://stackoverflow.com/questions/6017176/gllinestipple-deprecated-in-opengl-3-1
    m_opengl.apply(LineStippleType::TWO);

    //terrain texture
    m_opengl.apply(XColor4d{Qt::white, 1.0f});

    if (layer > 0) {
        if (Config().canvas.drawUpperLayersTextured) {
            m_opengl.apply(XEnable{XOption::POLYGON_STIPPLE});
        } else {
            m_opengl.apply(XDisable{XOption::POLYGON_STIPPLE});
            m_opengl.apply(XColor4d{0.3, 0.3, 0.3, 0.6 - 0.2 * static_cast<double>(layer)});
            m_opengl.apply(XEnable{XOption::BLEND});
        }
    } else if (layer == 0) {
        m_opengl.apply(XColor4d{Qt::white, 0.9f});
        m_opengl.apply(XEnable{XOption::BLEND});
    }

    const RoadIndex roadIndex = getRoadIndex(*room);
    drawUpperLayers(room, layer, roadIndex, wantExtraDetail);

    //walls
    m_opengl.glTranslated(0, 0, 0.005);

    if (layer > 0) {
        m_opengl.apply(XDisable{XOption::POLYGON_STIPPLE});
        m_opengl.apply(XColor4d{0.3, 0.3, 0.3, 0.6});
        m_opengl.apply(XEnable{XOption::BLEND});
    } else {
        m_opengl.apply(XColor4d{Qt::black});
    }

    m_opengl.apply(XDevicePointSize{3.0});
    m_opengl.apply(XDeviceLineWidth{2.4});

    for (auto dir : ALL_EXITS_NESW) {
        drawExit(room, rooms, dir);
    }

    m_opengl.apply(XDevicePointSize{3.0});
    m_opengl.apply(XDeviceLineWidth{2.0});

    for (auto dir : {ExitDirection ::UP, ExitDirection::DOWN}) {
        const auto &exitList = m_gllist.exit;
        const auto &updown = dir == ExitDirection::UP ? exitList.up : exitList.down;
        drawVertical(room, rooms, layer, dir, updown, m_gllist.door[dir]);
    }

    if (layer > 0) {
        m_opengl.apply(XDisable{XOption::BLEND});
    }

    m_opengl.glTranslated(0, 0, 0.0100);
    if (layer < 0) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XColor4d{Qt::black, 0.5 - 0.03 * static_cast<double>(layer)});
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        m_opengl.apply(XDisable{XOption::BLEND});
    } else if (layer > 0) {
        m_opengl.apply(XDisable{XOption::LINE_STIPPLE});

        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XColor4d{Qt::white, 0.1f}); // was 1.0f, 1.0f, 1.0f, 0.1f
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        m_opengl.apply(XDisable{XOption::BLEND});
    }

    if (!locks[room->getId()].empty()) { //---> room is locked
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::DEPTH_TEST});
        m_opengl.apply(XColor4d{0.6f, 0.0f, 0.0f, 0.2f});
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XEnable{XOption::DEPTH_TEST});
        m_opengl.apply(XDisable{XOption::BLEND});
    }

    m_opengl.glPopMatrix();

    if (wantExtraDetail) {
        drawRoomConnectionsAndDoors(room, rooms);
    }
}

void MapCanvasRoomDrawer::drawUpperLayers(const Room *const room,
                                          const qint32 layer,
                                          const RoadIndex &roadIndex,
                                          const bool wantExtraDetail)
{
    if (layer > 0 && !Config().canvas.drawUpperLayersTextured) {
        m_opengl.apply(XEnable{XOption::BLEND});
        m_opengl.callList(m_gllist.room);
        m_opengl.apply(XDisable{XOption::BLEND});
        return;
    }

    const auto roomTerrainType = room->getTerrainType();
    QOpenGLTexture *const texture = (roomTerrainType == RoomTerrainType::ROAD)
                                        ? m_textures.road[roadIndex]
                                        : m_textures.terrain[roomTerrainType];

    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XEnable{XOption::TEXTURE_2D});
    texture->bind();
    m_opengl.callList(m_gllist.room);

    // Make dark and troll safe rooms look dark
    if (room->getSundeathType() == RoomSundeathType::NO_SUNDEATH
        || room->getLightType() == RoomLightType::DARK) {
        SAVE_COLOR();
        m_opengl.glTranslated(0, 0, 0.005);
        m_opengl.apply(
            XColor4d{0.1f, 0.0f, 0.0f, room->getLightType() == RoomLightType::DARK ? 0.4f : 0.2f});
        m_opengl.callList(m_gllist.room);
    }

    // Only display at a certain scale
    if (wantExtraDetail) {
        const RoomMobFlags mf = room->getMobFlags();
        const RoomLoadFlags lf = room->getLoadFlags();

        // Draw a little dark red cross on noride rooms
        if (room->getRidableType() == RoomRidableType::NOT_RIDABLE) {
            SAVE_COLOR();
            m_opengl.apply(XDisable{XOption::TEXTURE_2D});

            m_opengl.apply(XColor4d{0.5f, 0.0f, 0.0f, 0.9f});
            m_opengl.apply(XDeviceLineWidth{3.0});
            m_opengl.draw(DrawType::LINES,
                          std::vector<Vec3d>{
                              Vec3d{0.6, 0.2, 0.005},
                              Vec3d{0.8, 0.4, 0.005},
                              Vec3d{0.8, 0.2, 0.005},
                              Vec3d{0.6, 0.4, 0.005},
                          });

            m_opengl.apply(XEnable{XOption::TEXTURE_2D});
        }

        // Trail Support
        m_opengl.glTranslated(0, 0, 0.005);
        if (roadIndex != RoadIndex::NONE && roomTerrainType != RoomTerrainType::ROAD) {
            alphaOverlayTexture(m_textures.trail[roadIndex]);
            m_opengl.glTranslated(0, 0, 0.005);
        }

        for (const RoomMobFlag flag : ALL_MOB_FLAGS)
            if (mf.contains(flag))
                alphaOverlayTexture(m_textures.mob[flag]);

        m_opengl.glTranslated(0, 0, 0.005);
        for (const RoomLoadFlag flag : ALL_LOAD_FLAGS) {
            // everything starting at RLF_ATTENTION is raised by a tiny bit?
            if (flag == RoomLoadFlag::ATTENTION)
                m_opengl.glTranslated(0, 0, 0.005);
            if (lf.contains(flag))
                alphaOverlayTexture(m_textures.load[flag]);
        }

        m_opengl.glTranslated(0, 0, 0.005);
        if (Config().canvas.showUpdated && !room->isUpToDate()) {
            alphaOverlayTexture(m_textures.update);
        }
        m_opengl.apply(XDisable{XOption::BLEND});
        m_opengl.apply(XDisable{XOption::TEXTURE_2D});
    }
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
    int rx = 0;
    int ry = 0;

    const auto wantDoorNames = Config().canvas.drawDoorNames && (m_scaleFactor >= 0.40f);
    for (const auto i : ALL_EXITS7) {
        const auto opp = opposite(i);
        ExitDirection targetDir = opp;
        const Exit &sourceExit = exitslist[i];
        //outgoing connections
        for (const auto targetId : sourceExit.outRange()) {
            targetRoom = rooms[targetId];
            rx = targetRoom->getPosition().x;
            ry = targetRoom->getPosition().y;
            if ((targetId >= sourceId) || // draw exits if targetId >= sourceId ...
                                          // or if target room is not visible
                (((static_cast<float>(rx) < m_visible1.x - 1.0f)
                  || (static_cast<float>(rx) > m_visible2.x + 1.0f))
                 || ((static_cast<float>(ry) < m_visible1.y - 1.0f)
                     || (static_cast<float>(ry) > m_visible2.y + 1.0f)))) {
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
            } else if (!sourceExit.containsIn(targetId)) { // ... or if they are outgoing oneways
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

            //incoming connections (only for oneway connections from rooms, that are not visible)
            for (const auto targetId : exitslist[i].inRange()) {
                targetRoom = rooms[targetId];
                rx = targetRoom->getPosition().x;
                ry = targetRoom->getPosition().y;

                if (((static_cast<float>(rx) < m_visible1.x - 1.0f)
                     || (static_cast<float>(rx) > m_visible2.x + 1.0f))
                    || ((static_cast<float>(ry) < m_visible1.y - 1.0f)
                        || (static_cast<float>(ry) > m_visible2.y + 1.0f))) {
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
            if (wantDoorNames && room->exit(i).isDoor() && room->exit(i).hasDoorName()) {
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

    const auto drawNotMappedExits = Config().canvas.drawNotMappedExits;
    if (drawNotMappedExits && roomExit.outIsEmpty()) { // zero outgoing connections
        drawListWithLineStipple(transparent, WALL_COLOR_NOTMAPPED);
        return;
    }

    const auto color = getVerticalColor(flags, g_noFleeColor);
    if (color != Qt::transparent)
        drawListWithLineStipple(transparent, color);

    /* NOTE: semi-bugfix: The opaque display list modifies color to black,
     * but the transparent display list doesn't.
     * Door display list doesn't set its own color, but flow does. */
    const auto useTransparent = layer > 0;
    m_opengl.apply(useTransparent ? transparent : opaque, XColor4d{Qt::black});

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
    m_opengl.apply(XColor4d{color});
    m_opengl.callList(list);
    m_opengl.apply(XColor4d{Qt::black});
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
    m_opengl.glTranslated(leftX - 0.5, leftY - 0.5, 0.0);

    m_opengl.apply(XColor4d{inExitFlags ? Qt::white : Qt::red, 0.70});

    m_opengl.apply(XEnable{XOption::BLEND});
    m_opengl.apply(XDevicePointSize{2.0});
    m_opengl.apply(XDeviceLineWidth{2.0});

    const double srcZ = ROOM_Z_DISTANCE * static_cast<double>(leftLayer) + 0.3;
    const double dstZ = ROOM_Z_DISTANCE * static_cast<double>(rightLayer) + 0.3;

    drawConnectionLine(startDir, endDir, oneway, neighbours, dX, dY, srcZ, dstZ);
    drawConnectionTriangles(startDir, endDir, oneway, dX, dY, srcZ, dstZ);

    m_opengl.apply(XDisable{XOption::BLEND});
    m_opengl.apply(XColor4d{Qt::white, 0.70});
    m_opengl.glPopMatrix();
}

void MapCanvasRoomDrawer::drawConnectionTriangles(const ExitDirection startDir,
                                                  const ExitDirection endDir,
                                                  const bool oneway,
                                                  const qint32 dX,
                                                  const qint32 dY,
                                                  const double srcZ,
                                                  const double dstZ)
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
                                             const double srcZ,
                                             const double dstZ)
{
    std::vector<Vec3d> points{};
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

void MapCanvasRoomDrawer::drawLineStrip(const std::vector<Vec3d> &points)
{
    m_opengl.draw(DrawType::LINE_STRIP, points);
}

void MapCanvasRoomDrawer::drawConnStartTri(const ExitDirection startDir, const double srcZ)
{
    switch (startDir) {
    case ExitDirection::NORTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{0.68, +0.1, srcZ},
                          Vec3d{0.82, +0.1, srcZ},
                          Vec3d{0.75, +0.3, srcZ},
                      });
        break;
    case ExitDirection::SOUTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{0.18, 0.9, srcZ},
                          Vec3d{0.32, 0.9, srcZ},
                          Vec3d{0.25, 0.7, srcZ},
                      });
        break;
    case ExitDirection::EAST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{0.9, 0.18, srcZ},
                          Vec3d{0.9, 0.32, srcZ},
                          Vec3d{0.7, 0.25, srcZ},
                      });
        break;
    case ExitDirection::WEST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{0.1, 0.68, srcZ},
                          Vec3d{0.1, 0.82, srcZ},
                          Vec3d{0.3, 0.75, srcZ},
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
                                         const double dstZ)
{
    switch (endDir) {
    case ExitDirection::NORTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.68, dY + 0.1, dstZ},
                          Vec3d{dX + 0.82, dY + 0.1, dstZ},
                          Vec3d{dX + 0.75, dY + 0.3, dstZ},
                      });
        break;
    case ExitDirection::SOUTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.18, dY + 0.9, dstZ},
                          Vec3d{dX + 0.32, dY + 0.9, dstZ},
                          Vec3d{dX + 0.25, dY + 0.7, dstZ},
                      });
        break;
    case ExitDirection::EAST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.9, dY + 0.18, dstZ},
                          Vec3d{dX + 0.9, dY + 0.32, dstZ},
                          Vec3d{dX + 0.7, dY + 0.25, dstZ},
                      });
        break;
    case ExitDirection::WEST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.1, dY + 0.68, dstZ},
                          Vec3d{dX + 0.1, dY + 0.82, dstZ},
                          Vec3d{dX + 0.3, dY + 0.75, dstZ},
                      });
        break;

    case ExitDirection::UP:
    case ExitDirection::DOWN:
        break;

    case ExitDirection::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUnknown(dX, dY, dstZ);
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
                                             const double dstZ)
{
    switch (endDir) {
    case ExitDirection::NORTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.18, dY + 0.1, dstZ},
                          Vec3d{dX + 0.32, dY + 0.1, dstZ},
                          Vec3d{dX + 0.25, dY + 0.3, dstZ},
                      });
        break;
    case ExitDirection::SOUTH:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.68, dY + 0.9, dstZ},
                          Vec3d{dX + 0.82, dY + 0.9, dstZ},
                          Vec3d{dX + 0.75, dY + 0.7, dstZ},
                      });
        break;
    case ExitDirection::EAST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.9, dY + 0.68, dstZ},
                          Vec3d{dX + 0.9, dY + 0.82, dstZ},
                          Vec3d{dX + 0.7, dY + 0.75, dstZ},
                      });
        break;
    case ExitDirection::WEST:
        m_opengl.draw(DrawType::TRIANGLES,
                      std::vector<Vec3d>{
                          Vec3d{dX + 0.1, dY + 0.18, dstZ},
                          Vec3d{dX + 0.1, dY + 0.32, dstZ},
                          Vec3d{dX + 0.3, dY + 0.25, dstZ},
                      });
        break;

    case ExitDirection::UP:
    case ExitDirection::DOWN:
        break;

    case ExitDirection::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUnknown(dX, dY, dstZ);
        break;
    case ExitDirection::NONE:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriNone(dX, dY, dstZ);
        break;
    }
}

void MapCanvasRoomDrawer::drawConnEndTriNone(qint32 dX, qint32 dY, double dstZ)
{
    m_opengl.draw(DrawType::TRIANGLES,
                  std::vector<Vec3d>{
                      Vec3d{dX + 0.5, dY + 0.5, dstZ},
                      Vec3d{dX + 0.7, dY + 0.55, dstZ},
                      Vec3d{dX + 0.55, dY + 0.7, dstZ},
                  });
}

void MapCanvasRoomDrawer::drawConnEndTriUnknown(qint32 dX, qint32 dY, double dstZ)
{
    m_opengl.draw(DrawType::TRIANGLES,
                  std::vector<Vec3d>{
                      Vec3d{dX + 0.5, dY + 0.5, dstZ},
                      Vec3d{dX + 0.7, dY + 0.55, dstZ},
                      Vec3d{dX + 0.55, dY + 0.7, dstZ},
                  });
}

void MapCanvasRoomDrawer::renderText(const float x,
                                     const float y,
                                     const QString &text,
                                     const QColor &color,
                                     const FontFormatFlags fontFormatFlag,
                                     const double rotationAngle)
{
    // http://stackoverflow.com/questions/28216001/how-to-render-text-with-qopenglwidget/28517897
    const QVector3D projected = project(QVector3D{x, y, CAMERA_Z_DISTANCE});
    const float textPosX = projected.x();
    const float textPosY = static_cast<float>(height()) - projected.y(); // y is inverted
    m_opengl.renderTextAt(textPosX, textPosY, text, color, fontFormatFlag, rotationAngle);
}
