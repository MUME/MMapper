// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Connections.h"

#include "../configuration/configuration.h"
#include "../global/Flags.h"
#include "../map/DoorFlags.h"
#include "../map/ExitFieldVariant.h"
#include "../map/exit.h"
#include "../map/room.h"
#include "../mapdata/mapdata.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "ConnectionLineBuilder.h"
#include "MapCanvasData.h"
#include "connectionselection.h"
#include "mapcanvas.h"

#include <cassert>
#include <cstdlib>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include <QColor>
#include <QMessageLogContext>
#include <QtCore>

static constexpr const float CONNECTION_LINE_WIDTH = 2.f;
static constexpr const float VALID_CONNECTION_POINT_SIZE = 6.f;
static constexpr const float NEW_CONNECTION_POINT_SIZE = 8.f;

static constexpr const float FAINT_CONNECTION_ALPHA = 0.1f;

NODISCARD static bool isConnectionMode(const CanvasMouseModeEnum mode)
{
    switch (mode) {
    case CanvasMouseModeEnum::CREATE_CONNECTIONS:
    case CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS:
    case CanvasMouseModeEnum::SELECT_CONNECTIONS:
        return true;
    case CanvasMouseModeEnum::NONE:
    case CanvasMouseModeEnum::MOVE:
    case CanvasMouseModeEnum::RAYPICK_ROOMS:
    case CanvasMouseModeEnum::SELECT_ROOMS:
    case CanvasMouseModeEnum::CREATE_ROOMS:
    case CanvasMouseModeEnum::SELECT_INFOMARKS:
    case CanvasMouseModeEnum::CREATE_INFOMARKS:
        break;
    }
    return false;
}

NODISCARD static glm::vec2 getConnectionOffsetRelative(const ExitDirEnum dir)
{
    switch (dir) {
    // NOTE: These are flipped north/south.
    case ExitDirEnum::NORTH:
        return {0.f, +0.4f};
    case ExitDirEnum::SOUTH:
        return {0.f, -0.4f};

    case ExitDirEnum::EAST:
        return {0.4f, 0.f};
    case ExitDirEnum::WEST:
        return {-0.4f, 0.f};

        // NOTE: These are flipped north/south.
    case ExitDirEnum::UP:
        return {0.25f, 0.25f};
    case ExitDirEnum::DOWN:
        return {-0.25f, -0.25f};

    case ExitDirEnum::UNKNOWN:
        return {0.f, 0.f};

    case ExitDirEnum::NONE:
    default:
        break;
    }

    assert(false);
    return {};
};

NODISCARD static glm::vec3 getConnectionOffset(const ExitDirEnum dir)
{
    return glm::vec3{getConnectionOffsetRelative(dir), 0.f} + glm::vec3{0.5f, 0.5f, 0.f};
}

NODISCARD static glm::vec3 getPosition(const ConnectionSelection::ConnectionDescriptor &cd)
{
    return deref(cd.room).getPosition().to_vec3() + getConnectionOffset(cd.direction);
}

NODISCARD static QString getDoorPostFix(const Room *const room, const ExitDirEnum dir)
{
    static constexpr const auto SHOWN_FLAGS = DoorFlagEnum::NEED_KEY | DoorFlagEnum::NO_PICK
                                              | DoorFlagEnum::DELAYED;

    const DoorFlags flags = room->exit(dir).getDoorFlags();
    if (!flags.containsAny(SHOWN_FLAGS))
        return QString{};

    return QString::asprintf(" [%s%s%s]",
                             flags.needsKey() ? "L" : "",
                             flags.isNoPick() ? "/NP" : "",
                             flags.isDelayed() ? "d" : "");
}

NODISCARD static QString getPostfixedDoorName(const Room *const room, const ExitDirEnum dir)
{
    const auto postFix = getDoorPostFix(room, dir);
    return room->exit(dir).getDoorName().toQString() + postFix;
}

UniqueMesh RoomNameBatch::getMesh(GLFont &font)
{
    return font.getFontMesh(m_names);
}

void ConnectionDrawer::drawRoomDoorName(const Room *const sourceRoom,
                                        const ExitDirEnum sourceDir,
                                        const Room *const targetRoom,
                                        const ExitDirEnum targetDir)
{
    static const auto isShortDistance = [](const Coordinate &a, const Coordinate &b) -> bool {
        return glm::all(glm::lessThanEqual(glm::abs(b.to_ivec2() - a.to_ivec2()), glm::ivec2(1)));
    };

    assert(sourceRoom != nullptr);
    assert(targetRoom != nullptr);

    const Coordinate &sourcePos = sourceRoom->getPosition();
    const Coordinate &targetPos = targetRoom->getPosition();

    if (sourcePos.z != m_currentLayer && targetPos.z != m_currentLayer) {
        return;
    }

    // consider converting this to std::string
    QString name;
    bool together = false;

    if (targetRoom->exit(targetDir).isDoor()          // the other room has a door?
        && targetRoom->exit(targetDir).hasDoorName()  // has a door on both sides...
        && targetRoom->exit(targetDir).isHiddenExit() // is hidden
        && isShortDistance(sourcePos, targetPos)) {
        if (sourceRoom->getId() > targetRoom->getId() && sourcePos.z == targetPos.z) {
            // NOTE: This allows wrap-around connections to the same room (allows source <= target).
            // Avoid drawing duplicate door names for each side by only drawing one side unless
            // the doors are on different z-layers
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

    static constexpr float XOFFSET = 0.6f;
    static const auto getYOffset = [](const ExitDirEnum dir) -> float {
        switch (dir) {
        case ExitDirEnum::NORTH:
            return 0.85f;
        case ExitDirEnum::SOUTH:
            return 0.35f;

        case ExitDirEnum::WEST:
            return 0.7f;
        case ExitDirEnum::EAST:
            return 0.55f;

        case ExitDirEnum::UP:
            return 1.05f;
        case ExitDirEnum::DOWN:
            return 0.2f;

        case ExitDirEnum::UNKNOWN:
        case ExitDirEnum::NONE:
            break;
        }

        assert(false);
        return 0.f;
    };

    const glm::vec2 xy = [sourceDir, together, &sourcePos, &targetPos]() -> glm::vec2 {
        const glm::vec2 srcPos = sourcePos.to_vec2();
        if (together) {
            const auto centerPos = (srcPos + targetPos.to_vec2()) * 0.5f;
            static constexpr float YOFFSET = 0.7f;
            return centerPos + glm::vec2{XOFFSET, YOFFSET};
        } else {
            return srcPos + glm::vec2{XOFFSET, getYOffset(sourceDir)};
        }
    }();

    static const auto bg = Colors::black.withAlpha(0.4f);
    const glm::vec3 pos{xy, m_currentLayer};
    m_roomNameBatch.emplace_back(GLText{pos,
                                        mmqt::toStdStringLatin1(name),
                                        Colors::white,
                                        bg,
                                        FontFormatFlags{FontFormatFlagEnum::HALIGN_CENTER}});
}

void ConnectionDrawer::drawRoomConnectionsAndDoors(const Room *const room, const RoomIndex &rooms)
{
    // Ooops, this is wrong since we may reject a connection that would be visible
    // if we looked at the other side.
    const bool sourceWithinBounds = m_bounds.contains(room->getPosition());

    const auto sourceId = room->getId();
    const ExitsList &exitslist = room->getExitsList();

    for (const ExitDirEnum sourceDir : ALL_EXITS7) {
        const Exit &sourceExit = exitslist[sourceDir];
        // outgoing connections
        if (sourceWithinBounds) {
            for (const auto &outTargetId : sourceExit.outRange()) {
                const SharedConstRoom &sharedTargetRoom = rooms[outTargetId];
                if (!sharedTargetRoom) {
                    qWarning() << "Source room" << sourceId.asUint32() << "("
                               << room->getName().toQString() << ") has target room"
                               << outTargetId.asUint32() << "which does not exist!";
                    continue;
                }

                const Room *const targetRoom = sharedTargetRoom.get();

                const bool targetOutsideBounds = !m_bounds.contains(targetRoom->getPosition());

                // Two way means that the target room directly connects back to source room
                const ExitDirEnum targetDir = opposite(sourceDir);
                const Exit &targetExit = targetRoom->exit(targetDir);
                const bool twoway = targetExit.containsOut(sourceId)
                                    && sourceExit.containsIn(outTargetId) && !targetOutsideBounds;

                const bool drawBothZLayers = room->getPosition().z != targetRoom->getPosition().z;

                if (!twoway) {
                    // Always draw exits for rooms that are not twoway exits
                    drawConnection(room,
                                   targetRoom,
                                   sourceDir,
                                   targetDir,
                                   !twoway,
                                   sourceExit.isExit() && !targetOutsideBounds);

                } else if (sourceId <= outTargetId || drawBothZLayers) {
                    // Avoid drawing duplicate exits for each side by only drawing one side
                    drawConnection(room,
                                   targetRoom,
                                   sourceDir,
                                   targetDir,
                                   !twoway,
                                   sourceExit.isExit() && targetExit.isExit());
                }

                // Draw door names
                if (sourceExit.isDoor() && sourceExit.hasDoorName() && sourceExit.isHiddenExit()) {
                    drawRoomDoorName(room, sourceDir, targetRoom, targetDir);
                }
            }
        }

        // incoming connections
        for (const auto &inTargetId : sourceExit.inRange()) {
            const SharedConstRoom &sharedTargetRoom = rooms[inTargetId];
            if (!sharedTargetRoom) {
                qWarning() << "Source room" << sourceId.asUint32() << "("
                           << room->getName().toQString() << ") has target room"
                           << inTargetId.asUint32() << "which does not exist!";
                continue;
            }

            const Room *const targetRoom = sharedTargetRoom.get();

            // Only draw the connection if the target room is within the bounds
            if (!m_bounds.contains(targetRoom->getPosition()))
                continue;

            // Only draw incoming connections if they are on a different layer
            if (room->getPosition().z == targetRoom->getPosition().z)
                continue;

            // Detect if this is a oneway
            bool oneway = true;
            for (const ExitDirEnum tempSourceDir : ALL_EXITS7) {
                const Exit &tempSourceExit = exitslist[tempSourceDir];
                if (tempSourceExit.containsOut(inTargetId)) {
                    oneway = false;
                }
            }
            if (oneway) {
                // Always draw one way connections for each target exit to the source room
                for (const ExitDirEnum targetDir : ALL_EXITS7) {
                    const Exit &targetExit = targetRoom->exit(targetDir);
                    if (targetExit.containsOut(sourceId)) {
                        drawConnection(targetRoom,
                                       room,
                                       targetDir,
                                       sourceDir,
                                       oneway,
                                       targetExit.isExit());
                    }
                }
            }
        }
    }
}

void ConnectionDrawer::drawConnection(const Room *leftRoom,
                                      const Room *rightRoom,
                                      ExitDirEnum startDir,
                                      ExitDirEnum endDir,
                                      bool oneway,
                                      bool inExitFlags)
{
    assert(leftRoom != nullptr);
    assert(rightRoom != nullptr);

    /* WARNING: attempts to write to a different layer may result in weird graphical bugs. */
    const Coordinate &leftPos = leftRoom->getPosition();
    const Coordinate &rightPos = rightRoom->getPosition();
    const int leftX = leftPos.x;
    const int leftY = leftPos.y;
    const int leftZ = leftPos.z;
    const int rightX = rightPos.x;
    const int rightY = rightPos.y;
    const int rightZ = rightPos.z;
    const int dX = rightX - leftX;
    const int dY = rightY - leftY;
    const int dZ = rightZ - leftZ;

    if ((rightZ != m_currentLayer) && (leftZ != m_currentLayer)) {
        return;
    }

    bool neighbours = false;

    if ((dX == 0) && (dY == 1) && (dZ == 0)) {
        if ((startDir == ExitDirEnum::NORTH) && (endDir == ExitDirEnum::SOUTH) && !oneway) {
            return;
        }
        neighbours = true;
    }
    if ((dX == 0) && (dY == -1) && (dZ == 0)) {
        if ((startDir == ExitDirEnum::SOUTH) && (endDir == ExitDirEnum::NORTH) && !oneway) {
            return;
        }
        neighbours = true;
    }
    if ((dX == +1) && (dY == 0) && (dZ == 0)) {
        if ((startDir == ExitDirEnum::EAST) && (endDir == ExitDirEnum::WEST) && !oneway) {
            return;
        }
        neighbours = true;
    }
    if ((dX == -1) && (dY == 0) && (dZ == 0)) {
        if ((startDir == ExitDirEnum::WEST) && (endDir == ExitDirEnum::EAST) && !oneway) {
            return;
        }
        neighbours = true;
    }

    auto &gl = getFakeGL();

    gl.setOffset(static_cast<float>(leftX), static_cast<float>(leftY), 0.f);

    if (inExitFlags)
        gl.setNormal();
    else
        gl.setRed();

    {
        const auto srcZ = static_cast<float>(leftZ);
        const auto dstZ = static_cast<float>(rightZ);

        drawConnectionLine(startDir,
                           endDir,
                           oneway,
                           neighbours,
                           static_cast<float>(dX),
                           static_cast<float>(dY),
                           srcZ,
                           dstZ);
        drawConnectionTriangles(startDir,
                                endDir,
                                oneway,
                                static_cast<float>(dX),
                                static_cast<float>(dY),
                                srcZ,
                                dstZ);
    }

    gl.setOffset(0, 0, 0);
    gl.setNormal();
}

void ConnectionDrawer::drawConnectionTriangles(const ExitDirEnum startDir,
                                               const ExitDirEnum endDir,
                                               const bool oneway,
                                               const float dX,
                                               const float dY,
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

void ConnectionDrawer::drawConnectionLine(const ExitDirEnum startDir,
                                          const ExitDirEnum endDir,
                                          const bool oneway,
                                          const bool neighbours,
                                          const float dX,
                                          const float dY,
                                          const float srcZ,
                                          const float dstZ)
{
    std::vector<glm::vec3> points{};
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

void ConnectionDrawer::drawLineStrip(const std::vector<glm::vec3> &points)
{
    getFakeGL().drawLineStrip(points);
}

void ConnectionDrawer::drawConnStartTri(const ExitDirEnum startDir, const float srcZ)
{
    auto &gl = getFakeGL();

    switch (startDir) {
    case ExitDirEnum::NORTH:
        gl.drawTriangle(glm::vec3{0.82f, 0.9f, srcZ},
                        glm::vec3{0.68f, 0.9f, srcZ},
                        glm::vec3{0.75f, 0.7f, srcZ});
        break;
    case ExitDirEnum::SOUTH:
        gl.drawTriangle(glm::vec3{0.18f, 0.1f, srcZ},
                        glm::vec3{0.32f, 0.1f, srcZ},
                        glm::vec3{0.25f, 0.3f, srcZ});
        break;
    case ExitDirEnum::EAST:
        gl.drawTriangle(glm::vec3{0.9f, 0.68f, srcZ},
                        glm::vec3{0.9f, 0.82f, srcZ},
                        glm::vec3{0.7f, 0.75f, srcZ});
        break;
    case ExitDirEnum::WEST:
        gl.drawTriangle(glm::vec3{0.1f, 0.32f, srcZ},
                        glm::vec3{0.1f, 0.18f, srcZ},
                        glm::vec3{0.3f, 0.25f, srcZ});
        break;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        // Do not draw triangles for 2-way up/down
        break;
    case ExitDirEnum::UNKNOWN:
        drawConnEndTriUpDownUnknown(0, 0, srcZ);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}

void ConnectionDrawer::drawConnEndTri(const ExitDirEnum endDir,
                                      const float dX,
                                      const float dY,
                                      const float dstZ)
{
    auto &gl = getFakeGL();

    switch (endDir) {
    case ExitDirEnum::NORTH:
        gl.drawTriangle(glm::vec3{dX + 0.82f, dY + 0.9f, dstZ},
                        glm::vec3{dX + 0.68f, dY + 0.9f, dstZ},
                        glm::vec3{dX + 0.75f, dY + 0.7f, dstZ});
        break;
    case ExitDirEnum::SOUTH:
        gl.drawTriangle(glm::vec3{dX + 0.18f, dY + 0.1f, dstZ},
                        glm::vec3{dX + 0.32f, dY + 0.1f, dstZ},
                        glm::vec3{dX + 0.25f, dY + 0.3f, dstZ});
        break;
    case ExitDirEnum::EAST:
        gl.drawTriangle(glm::vec3{dX + 0.9f, dY + 0.68f, dstZ},
                        glm::vec3{dX + 0.9f, dY + 0.82f, dstZ},
                        glm::vec3{dX + 0.7f, dY + 0.75f, dstZ});
        break;
    case ExitDirEnum::WEST:
        gl.drawTriangle(glm::vec3{dX + 0.1f, dY + 0.32f, dstZ},
                        glm::vec3{dX + 0.1f, dY + 0.18f, dstZ},
                        glm::vec3{dX + 0.3f, dY + 0.25f, dstZ});
        break;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        // Do not draw triangles for 2-way up/down
        break;
    case ExitDirEnum::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUpDownUnknown(dX, dY, dstZ);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}

void ConnectionDrawer::drawConnEndTri1Way(const ExitDirEnum endDir,
                                          const float dX,
                                          const float dY,
                                          const float dstZ)
{
    auto &gl = getFakeGL();

    switch (endDir) {
    case ExitDirEnum::NORTH:
        gl.drawTriangle(glm::vec3{dX + 0.32f, dY + 0.9f, dstZ},
                        glm::vec3{dX + 0.18f, dY + 0.9f, dstZ},
                        glm::vec3{dX + 0.25f, dY + 0.7f, dstZ});
        break;
    case ExitDirEnum::SOUTH:
        gl.drawTriangle(glm::vec3{dX + 0.68f, dY + 0.1f, dstZ},
                        glm::vec3{dX + 0.82f, dY + 0.1f, dstZ},
                        glm::vec3{dX + 0.75f, dY + 0.3f, dstZ});
        break;
    case ExitDirEnum::EAST:
        gl.drawTriangle(glm::vec3{dX + 0.9f, dY + 0.18f, dstZ},
                        glm::vec3{dX + 0.9f, dY + 0.32f, dstZ},
                        glm::vec3{dX + 0.7f, dY + 0.25f, dstZ});
        break;
    case ExitDirEnum::WEST:
        gl.drawTriangle(glm::vec3{dX + 0.1f, dY + 0.82f, dstZ},
                        glm::vec3{dX + 0.1f, dY + 0.68f, dstZ},
                        glm::vec3{dX + 0.3f, dY + 0.75f, dstZ});
        break;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
    case ExitDirEnum::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUpDownUnknown(dX, dY, dstZ);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}

void ConnectionDrawer::drawConnEndTriUpDownUnknown(float dX, float dY, float dstZ)
{
    getFakeGL().drawTriangle(glm::vec3{dX + 0.5f, dY + 0.5f, dstZ},
                             glm::vec3{dX + 0.55f, dY + 0.3f, dstZ},
                             glm::vec3{dX + 0.7f, dY + 0.45f, dstZ});
}

ConnectionMeshes ConnectionDrawerBuffers::getMeshes(OpenGL &gl)
{
    ConnectionMeshes result;
    result.normalLines = gl.createColoredLineBatch(normal.lineVerts);
    result.normalTris = gl.createColoredTriBatch(normal.triVerts);
    result.redLines = gl.createColoredLineBatch(red.lineVerts);
    result.redTris = gl.createColoredTriBatch(red.triVerts);
    return result;
}

void ConnectionMeshes::render(const int thisLayer, const int focusedLayer)
{
    const auto color = [&thisLayer, &focusedLayer]() {
        if (thisLayer == focusedLayer) {
            return getConfig().canvas.connectionNormalColor.getColor();
        }
        return Colors::gray70.withAlpha(FAINT_CONNECTION_ALPHA);
    }();
    const auto common_style = GLRenderState()
                                  .withBlend(BlendModeEnum::TRANSPARENCY)
                                  .withLineParams(LineParams{CONNECTION_LINE_WIDTH})
                                  .withColor(color);

    // Even though we can draw colored lines and tris,
    // the reason for having separate lines is so red will always be on top.
    // If you don't think that's important, you can combine the batches.

    normalLines.render(common_style);
    normalTris.render(common_style);
    redLines.render(common_style);
    redTris.render(common_style);
}

void MapCanvas::paintNearbyConnectionPoints()
{
    const bool isSelection = m_canvasMouseMode == CanvasMouseModeEnum::SELECT_CONNECTIONS;
    using CD = ConnectionSelection::ConnectionDescriptor;

    static const ExitDirFlags allExits = []() {
        ExitDirFlags tmp;
        for (const ExitDirEnum dir : ALL_EXITS7) {
            tmp |= dir;
        }
        return tmp;
    }();

    std::vector<ColorVert> points;
    const auto addPoint = [isSelection, &points](const Coordinate &roomCoord,
                                                 const Room *const room,
                                                 const ExitDirEnum dir,
                                                 const std::optional<CD> &optFirst) -> void {
        if (!isNESWUD(dir) && dir != ExitDirEnum::UNKNOWN)
            return;

        if (optFirst) {
            const CD &first = optFirst.value();
            const CD second{room, dir};
            if (isSelection ? !CD::isCompleteExisting(first, second)
                            : !CD::isCompleteNew(first, second)) {
                return;
            }
        }

        points.emplace_back(Colors::cyan, roomCoord.to_vec3() + getConnectionOffset(dir));
    };
    const auto addPoints =
        [this, isSelection, &addPoint](const std::optional<MouseSel> &sel,
                                       const std::optional<CD> &optFirst) -> void {
        if (!sel.has_value())
            return;
        const auto mouse = sel->getCoordinate();
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const auto roomCoord = mouse + Coordinate(dx, dy, 0);
                const Room *const room = m_data.getRoom(roomCoord);
                if (room == nullptr)
                    continue;

                ExitDirFlags dirs = isSelection ? m_data.getExitDirections(roomCoord) : allExits;
                if (optFirst)
                    dirs |= ExitDirEnum::UNKNOWN;

                dirs.for_each([&addPoint, &optFirst, &roomCoord, room](ExitDirEnum dir) -> void {
                    addPoint(roomCoord, room, dir, optFirst);
                });
            }
        }
    };

    // FIXME: This doesn't show dots for red connections.
    if (m_connectionSelection != nullptr
        && (m_connectionSelection->isFirstValid() || m_connectionSelection->isSecondValid())) {
        const CD valid = m_connectionSelection->isFirstValid() ? m_connectionSelection->getFirst()
                                                               : m_connectionSelection->getSecond();
        const Coordinate &c = deref(valid.room).getPosition();
        const glm::vec3 &pos = c.to_vec3();
        points.emplace_back(Colors::cyan, pos + getConnectionOffset(valid.direction));

        addPoints(MouseSel{Coordinate2f{pos.x, pos.y}, c.z}, valid);
        addPoints(m_sel1, valid);
        addPoints(m_sel2, valid);
    } else {
        addPoints(m_sel1, std::nullopt);
        addPoints(m_sel2, std::nullopt);
    }

    getOpenGL().renderPoints(points, GLRenderState().withPointSize(VALID_CONNECTION_POINT_SIZE));
}

void MapCanvas::paintSelectedConnection()
{
    if (isConnectionMode(m_canvasMouseMode)) {
        paintNearbyConnectionPoints();
    }

    if (m_connectionSelection == nullptr || !m_connectionSelection->isFirstValid()) {
        return;
    }

    ConnectionSelection &sel = deref(m_connectionSelection);

    const ConnectionSelection::ConnectionDescriptor &first = sel.getFirst();
    const auto pos1 = getPosition(first);
    // REVISIT: How about not dashed lines to the nearest possible connections
    // if the second isn't valid?
    const auto optPos2 = [this, &sel]() -> std::optional<glm::vec3> {
        if (sel.isSecondValid())
            return getPosition(sel.getSecond());
        else if (hasSel2())
            return getSel2().to_vec3();
        else
            return std::nullopt;
    }();

    if (!optPos2)
        return;

    const glm::vec3 &pos2 = optPos2.value();

    auto &gl = getOpenGL();
    const auto rs = GLRenderState().withColor(Colors::red);

    const std::vector<glm::vec3> verts{pos1, pos2};
    gl.renderPlainLines(verts, rs.withLineParams(LineParams{CONNECTION_LINE_WIDTH}));

    std::vector<ColorVert> points;
    points.emplace_back(Colors::red, pos1);
    points.emplace_back(Colors::red, pos2);
    gl.renderPoints(points, rs.withPointSize(NEW_CONNECTION_POINT_SIZE));
}

static constexpr float LONG_LINE_HALFLEN = 1.5f;
static constexpr float LONG_LINE_LEN = 2.f * LONG_LINE_HALFLEN;
NODISCARD static bool isLongLine(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) >= LONG_LINE_LEN;
}

void ConnectionDrawer::ConnectionFakeGL::drawTriangle(const glm::vec3 &a,
                                                      const glm::vec3 &b,
                                                      const glm::vec3 &c)
{
    const auto &color = isNormal() ? getConfig().canvas.connectionNormalColor.getColor()
                                   : Colors::red;
    auto &verts = deref(m_currentBuffer).triVerts;
    verts.emplace_back(color, a + m_offset);
    verts.emplace_back(color, b + m_offset);
    verts.emplace_back(color, c + m_offset);
}

void ConnectionDrawer::ConnectionFakeGL::drawLineStrip(const std::vector<glm::vec3> &points)
{
    const auto &connectionNormalColor = isNormal()
                                            ? getConfig().canvas.connectionNormalColor.getColor()
                                            : Colors::red;

    const auto transform = [this](const glm::vec3 &vert) { return vert + m_offset; };
    auto &verts = deref(m_currentBuffer).lineVerts;
    auto drawLine = [&verts](const Color &color, const glm::vec3 &a, const glm::vec3 &b) {
        verts.emplace_back(color, a);
        verts.emplace_back(color, b);
    };

    const auto size = points.size();
    assert(size >= 2);
    for (size_t i = 1; i < size; ++i) {
        const auto start = transform(points[i - 1u]);
        const auto end = transform(points[i]);

        if (!isLongLine(start, end)) {
            drawLine(connectionNormalColor, start, end);
            continue;
        }

        const auto len = glm::length(start - end);
        const auto faintCutoff = LONG_LINE_HALFLEN / len;
        const auto mid1 = glm::mix(start, end, faintCutoff);
        const auto mid2 = glm::mix(start, end, 1.f - faintCutoff);
        const auto faint = connectionNormalColor.withAlpha(FAINT_CONNECTION_ALPHA);

        drawLine(connectionNormalColor, start, mid1);
        drawLine(faint, mid1, mid2);
        drawLine(connectionNormalColor, mid2, end);
    }
}
