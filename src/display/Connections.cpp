// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Connections.h"

#include "../configuration/configuration.h"
#include "../global/Flags.h"
#include "../map/DoorFlags.h"
#include "../map/ExitFieldVariant.h"
#include "../mapdata/mapdata.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/LineRendering.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "ConnectionLineBuilder.h"
#include "MapCanvasData.h"
#include "RoomRenderTransform.h"
#include "connectionselection.h"
#include "mapcanvas.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <QColor>
#include <QMessageLogContext>
#include <QtCore>

static constexpr const float CONNECTION_LINE_WIDTH = 0.045f;
static constexpr const float VALID_CONNECTION_POINT_SIZE = 6.f;
static constexpr const float NEW_CONNECTION_POINT_SIZE = 8.f;
static constexpr const float ROOM_SCALE_MARGIN = 0.4f;

static constexpr const float FAINT_CONNECTION_ALPHA = 0.1f;

NODISCARD static bool isCrossingZAxis(const glm::vec3 &p1, const glm::vec3 &p2)
{
    return std::abs(p1.z - p2.z) > mmgl::GEOMETRIC_EPSILON;
}

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

NODISCARD static float sanitizeRoomScale(const float scale)
{
    if (!std::isfinite(scale) || scale <= 0.f) {
        return 1.f;
    }
    return scale;
}

NODISCARD static bool isRoomWithinBounds(const RoomHandle &room, const OptBounds &bounds)
{
    if (!bounds.isRestricted()) {
        return true;
    }

    const auto &bb = bounds.getBounds();
    const auto transform = getRoomRenderTransform(room);
    const auto &center = transform.renderCenter;
    return center.x >= bb.min.x && center.x <= bb.max.x + 1.f && center.y >= bb.min.y
           && center.y <= bb.max.y + 1.f && center.z >= bb.min.z && center.z <= bb.max.z + 1.f;
}

NODISCARD static glm::vec2 getConnectionOffsetRelative(const ExitDirEnum dir, const float scale)
{
    const float roomScale = sanitizeRoomScale(scale);

    switch (dir) {
    // NOTE: These are flipped north/south.
    case ExitDirEnum::NORTH:
        return {0.f, +0.4f * roomScale};
    case ExitDirEnum::SOUTH:
        return {0.f, -0.4f * roomScale};

    case ExitDirEnum::EAST:
        return {0.4f * roomScale, 0.f};
    case ExitDirEnum::WEST:
        return {-0.4f * roomScale, 0.f};

        // NOTE: These are flipped north/south.
    case ExitDirEnum::UP:
        return {0.25f * roomScale, 0.25f * roomScale};
    case ExitDirEnum::DOWN:
        return {-0.25f * roomScale, -0.25f * roomScale};

    case ExitDirEnum::UNKNOWN:
        return {0.f, 0.f};

    case ExitDirEnum::NONE:
    default:
        break;
    }

    assert(false);
    return {};
};

NODISCARD static glm::vec3 getConnectionOffset(const ExitDirEnum dir, const float scale)
{
    return glm::vec3{getConnectionOffsetRelative(dir, scale), 0.f} + glm::vec3{0.5f, 0.5f, 0.f};
}

NODISCARD static glm::vec3 getConnectionOffset(const RoomHandle &room, const ExitDirEnum dir)
{
    return getConnectionOffset(dir, room.getScaleFactor());
}

NODISCARD static glm::vec3 getPosition(const ConnectionSelection::ConnectionDescriptor &cd)
{
    const auto transform = getRoomRenderTransform(cd.room);
    const glm::vec3 worldPos
        = cd.room.getPosition().to_vec3() + getConnectionOffset(cd.room, cd.direction);
    return applyLocalspaceTransform(transform, worldPos);
}

NODISCARD static glm::vec3 scalePointInRoom(const glm::vec3 &pos,
                                            const glm::vec2 &origin,
                                            const float scale)
{
    const glm::vec2 center = origin + glm::vec2{0.5f, 0.5f};
    const glm::vec2 xy = center + (glm::vec2{pos.x, pos.y} - center) * scale;
    return glm::vec3{xy, pos.z};
}

NODISCARD static bool isWithinRoomBounds(const glm::vec3 &pos,
                                         const glm::vec2 &origin,
                                         const float margin)
{
    return pos.x >= origin.x - margin && pos.x <= origin.x + 1.f + margin
           && pos.y >= origin.y - margin && pos.y <= origin.y + 1.f + margin;
}

NODISCARD static QString getDoorPostFix(const RoomHandle &room, const ExitDirEnum dir)
{
    static constexpr const auto SHOWN_FLAGS = DoorFlagEnum::NEED_KEY | DoorFlagEnum::NO_PICK
                                              | DoorFlagEnum::DELAYED;

    const DoorFlags flags = room.getExit(dir).getDoorFlags();
    if (!flags.containsAny(SHOWN_FLAGS)) {
        return QString{};
    }

    return QString::asprintf(" [%s%s%s]",
                             flags.needsKey() ? "L" : "",
                             flags.isNoPick() ? "/NP" : "",
                             flags.isDelayed() ? "d" : "");
}

NODISCARD static QString getPostfixedDoorName(const RoomHandle &room, const ExitDirEnum dir)
{
    const auto postFix = getDoorPostFix(room, dir);
    return room.getExit(dir).getDoorName().toQString() + postFix;
}

NODISCARD UniqueMesh RoomNameBatchIntermediate::getMesh(GLFont &gl) const
{
    return gl.getFontMesh(verts);
}

NODISCARD RoomNameBatchIntermediate RoomNameBatch::getIntermediate(const FontMetrics &font) const
{
    std::vector<FontVert3d> output;
    ::getFontBatchRawData(font, m_names.data(), m_names.size(), output);
    return RoomNameBatchIntermediate{std::move(output)};
}

void ConnectionDrawer::drawRoomDoorName(const RoomHandle &sourceRoom,
                                        const ExitDirEnum sourceDir,
                                        const RoomHandle &targetRoom,
                                        const ExitDirEnum targetDir)
{
    static const auto isShortDistance = [](const Coordinate &a, const Coordinate &b) -> bool {
        return glm::all(glm::lessThanEqual(glm::abs(b.to_ivec2() - a.to_ivec2()), glm::ivec2(1)));
    };

    const Coordinate &sourcePos = sourceRoom.getPosition();
    const Coordinate &targetPos = targetRoom.getPosition();
    const auto sourceTransform = getRoomRenderTransform(sourceRoom);
    const auto targetTransform = getRoomRenderTransform(targetRoom);

    if (sourcePos.z != m_currentLayer && targetPos.z != m_currentLayer) {
        return;
    }

    // consider converting this to std::string
    QString name;
    bool together = false;

    const auto &targetExit = targetRoom.getExit(targetDir);
    if (targetExit.exitIsDoor()      // the other room has a door?
        && targetExit.hasDoorName()  // has a door on both sides...
        && targetExit.doorIsHidden() // is hidden
        && isShortDistance(sourcePos, targetPos)) {
        if (sourceRoom.getId() > targetRoom.getId() && sourcePos.z == targetPos.z) {
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

    const glm::vec2 xy = std::invoke([sourceDir, together, &sourcePos, &targetPos]() -> glm::vec2 {
        const glm::vec2 srcPos = sourcePos.to_vec2();
        if (together) {
            const auto centerPos = (srcPos + targetPos.to_vec2()) * 0.5f;
            static constexpr float YOFFSET = 0.7f;
            return centerPos + glm::vec2{XOFFSET, YOFFSET};
        } else {
            return srcPos + glm::vec2{XOFFSET, getYOffset(sourceDir)};
        }
    });

    static const auto bg = Colors::black.withAlpha(0.4f);
    const glm::vec3 pos{xy, static_cast<float>(m_currentLayer)};
    const auto &transform = std::invoke([&]() -> const RoomRenderTransform & {
        if (sourceTransform.localspaceId == targetTransform.localspaceId) {
            return sourceTransform;
        }
        const float distSource = glm::distance(xy, sourcePos.to_vec2());
        const float distTarget = glm::distance(xy, targetPos.to_vec2());
        return (distSource <= distTarget) ? sourceTransform : targetTransform;
    });
    const glm::vec3 renderPos = applyLocalspaceTransform(transform, pos);
    m_roomNameBatch.emplace_back(GLText{renderPos,
                                        mmqt::toStdStringLatin1(name), // GL font is latin1
                                        Colors::white,
                                        bg,
                                        FontFormatFlags{FontFormatFlagEnum::HALIGN_CENTER}});
}

void ConnectionDrawer::drawRoomConnectionsAndDoors(const RoomHandle &room)
{
    const Map &map = room.getMap();

    // Ooops, this is wrong since we may reject a connection that would be visible
    // if we looked at the other side.
    const auto room_pos = room.getPosition();
    const bool sourceWithinBounds = isRoomWithinBounds(room, m_bounds);

    const auto sourceId = room.getId();

    for (const ExitDirEnum sourceDir : ALL_EXITS7) {
        const auto &sourceExit = room.getExit(sourceDir);
        // outgoing connections
        if (sourceWithinBounds) {
            for (const RoomId outTargetId : sourceExit.getOutgoingSet()) {
                const auto &targetRoom = map.getRoomHandle(outTargetId);
                if (!targetRoom) {
                    /* DEAD CODE */
                    qWarning() << "Source room" << sourceId.asUint32() << "("
                               << room.getName().toQString()
                               << ") dir=" << mmqt::toQStringUtf8(to_string_view(sourceDir))
                               << "has target room with internal identifier"
                               << outTargetId.asUint32() << "which does not exist!";
                    // This would cause a segfault in the old map scheme, but maps are now rigorously
                    // validated, so it should be impossible to have an exit to a room that does
                    // not exist.
                    assert(false);
                    continue;
                }
                const auto &target_coord = targetRoom.getPosition();
                const bool targetOutsideBounds = !isRoomWithinBounds(targetRoom, m_bounds);

                // Two way means that the target room directly connects back to source room
                const ExitDirEnum targetDir = opposite(sourceDir);
                const auto &targetExit = targetRoom.getExit(targetDir);
                const bool twoway = targetExit.containsOut(sourceId)
                                    && sourceExit.containsIn(outTargetId) && !targetOutsideBounds;

                const bool drawBothZLayers = room_pos.z != target_coord.z;

                if (!twoway) {
                    // Always draw exits for rooms that are not twoway exits
                    drawConnection(room,
                                   targetRoom,
                                   sourceDir,
                                   targetDir,
                                   !twoway,
                                   sourceExit.exitIsExit() && !targetOutsideBounds);

                } else if (sourceId <= outTargetId || drawBothZLayers) {
                    // Avoid drawing duplicate exits for each side by only drawing one side
                    drawConnection(room,
                                   targetRoom,
                                   sourceDir,
                                   targetDir,
                                   !twoway,
                                   sourceExit.exitIsExit() && targetExit.exitIsExit());
                }

                // Draw door names
                if (sourceExit.exitIsDoor() && sourceExit.hasDoorName()
                    && sourceExit.doorIsHidden()) {
                    drawRoomDoorName(room, sourceDir, targetRoom, targetDir);
                }
            }
        }

        // incoming connections
        for (const RoomId inTargetId : sourceExit.getIncomingSet()) {
            const auto &targetRoom = map.getRoomHandle(inTargetId);
            if (!targetRoom) {
                /* DEAD CODE */
                qWarning() << "Source room" << sourceId.asUint32() << "("
                           << room.getName().toQString() << ") fromdir="
                           << mmqt::toQStringUtf8(to_string_view(opposite(sourceDir)))
                           << " has target room with internal identifier" << inTargetId.asUint32()
                           << "which does not exist!";
                // This would cause a segfault in the old map scheme, but maps are now rigorously
                // validated, so it should be impossible to have an exit to a room that does
                // not exist.
                assert(false);
                continue;
            }

            // Only draw the connection if the target room is within the bounds
            const Coordinate &target_coord = targetRoom.getPosition();
            if (!isRoomWithinBounds(targetRoom, m_bounds)) {
                continue;
            }

            // Only draw incoming connections if they are on a different layer
            if (room_pos.z == target_coord.z) {
                continue;
            }

            // Detect if this is a oneway
            bool oneway = true;
            for (const auto &tempSourceExit : room.getExits()) {
                if (tempSourceExit.containsOut(inTargetId)) {
                    oneway = false;
                    break;
                }
            }
            if (oneway) {
                // Always draw one way connections for each target exit to the source room
                for (const ExitDirEnum targetDir : ALL_EXITS7) {
                    const auto &targetExit = targetRoom.getExit(targetDir);
                    if (targetExit.containsOut(sourceId)) {
                        drawConnection(targetRoom,
                                       room,
                                       targetDir,
                                       sourceDir,
                                       oneway,
                                       targetExit.exitIsExit());
                    }
                }
            }
        }
    }
}

void ConnectionDrawer::drawConnection(const RoomHandle &leftRoom,
                                      const RoomHandle &rightRoom,
                                      const ExitDirEnum startDir,
                                      const ExitDirEnum endDir,
                                      const bool oneway,
                                      const bool inExitFlags)
{
    /* WARNING: attempts to write to a different layer may result in weird graphical bugs. */
    const Coordinate &leftPos = leftRoom.getPosition();
    const Coordinate &rightPos = rightRoom.getPosition();
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

    const auto leftTransform = getRoomRenderTransform(leftRoom);
    const auto rightTransform = getRoomRenderTransform(rightRoom);
    const glm::vec3 leftOrigin = leftPos.to_vec3();
    const glm::vec2 startOrigin{0.f, 0.f};
    const glm::vec2 endOrigin{static_cast<float>(dX), static_cast<float>(dY)};
    const bool sameLocalspace = leftTransform.localspaceId == rightTransform.localspaceId;

    gl.setOffset(0.f, 0.f, 0.f);
    gl.setPointTransform([leftTransform,
                          rightTransform,
                          leftOrigin,
                          startOrigin,
                          endOrigin,
                          sameLocalspace](const glm::vec3 &point) -> glm::vec3 {
        const glm::vec3 worldPoint = point + leftOrigin;
        if (sameLocalspace) {
            return applyLocalspaceTransform(leftTransform, worldPoint);
        }
        if (isWithinRoomBounds(point, startOrigin, ROOM_SCALE_MARGIN)) {
            return applyLocalspaceTransform(leftTransform, worldPoint);
        }
        if (isWithinRoomBounds(point, endOrigin, ROOM_SCALE_MARGIN)) {
            return applyLocalspaceTransform(rightTransform, worldPoint);
        }
        const glm::vec2 worldXY{worldPoint.x, worldPoint.y};
        const glm::vec2 leftXY{leftTransform.roomCenter.x, leftTransform.roomCenter.y};
        const glm::vec2 rightXY{rightTransform.roomCenter.x, rightTransform.roomCenter.y};
        const float distLeft = glm::distance(worldXY, leftXY);
        const float distRight = glm::distance(worldXY, rightXY);
        const auto &transform = (distLeft <= distRight) ? leftTransform : rightTransform;
        return applyLocalspaceTransform(transform, worldPoint);
    });

    if (inExitFlags) {
        gl.setNormal();
    } else {
        gl.setRed();
    }

    {
        const auto srcZ = static_cast<float>(leftZ);
        const auto dstZ = static_cast<float>(rightZ);
        const float startScale = sanitizeRoomScale(leftRoom.getScaleFactor());
        const float endScale = sanitizeRoomScale(rightRoom.getScaleFactor());

        drawConnectionLine(startDir,
                           endDir,
                           oneway,
                           neighbours,
                           static_cast<float>(dX),
                           static_cast<float>(dY),
                           srcZ,
                           dstZ,
                           startScale,
                           endScale);
        drawConnectionTriangles(startDir,
                                endDir,
                                oneway,
                                static_cast<float>(dX),
                                static_cast<float>(dY),
                                srcZ,
                                dstZ,
                                startScale,
                                endScale);
    }

    gl.clearPointTransform();
    gl.setOffset(0, 0, 0);
    gl.setNormal();
}

void ConnectionDrawer::drawConnectionTriangles(const ExitDirEnum startDir,
                                               const ExitDirEnum endDir,
                                               const bool oneway,
                                               const float dX,
                                               const float dY,
                                               const float srcZ,
                                               const float dstZ,
                                               const float startScale,
                                               const float endScale)
{
    if (oneway) {
        drawConnEndTri1Way(endDir, dX, dY, dstZ, endScale);
    } else {
        drawConnStartTri(startDir, srcZ, startScale);
        drawConnEndTri(endDir, dX, dY, dstZ, endScale);
    }
}

void ConnectionDrawer::drawConnectionLine(const ExitDirEnum startDir,
                                          const ExitDirEnum endDir,
                                          const bool oneway,
                                          const bool neighbours,
                                          const float dX,
                                          const float dY,
                                          const float srcZ,
                                          const float dstZ,
                                          const float startScale,
                                          const float endScale)
{
    std::vector<glm::vec3> points{};
    ConnectionLineBuilder lb{points};
    lb.drawConnLineStart(startDir, neighbours, srcZ);
    if (points.empty()) {
        return;
    }

    if (oneway) {
        lb.drawConnLineEnd1Way(endDir, dX, dY, dstZ);
    } else {
        lb.drawConnLineEnd2Way(endDir, neighbours, dX, dY, dstZ);
    }

    if (points.empty()) {
        return;
    }

    const glm::vec2 startOrigin{0.f, 0.f};
    const glm::vec2 endOrigin{dX, dY};
    for (auto &point : points) {
        if (isWithinRoomBounds(point, startOrigin, ROOM_SCALE_MARGIN)) {
            point = scalePointInRoom(point, startOrigin, startScale);
        } else if (isWithinRoomBounds(point, endOrigin, ROOM_SCALE_MARGIN)) {
            point = scalePointInRoom(point, endOrigin, endScale);
        }
    }

    drawLineStrip(points);
}

void ConnectionDrawer::drawLineStrip(const std::vector<glm::vec3> &points)
{
    getFakeGL().drawLineStrip(points);
}

void ConnectionDrawer::drawConnStartTri(const ExitDirEnum startDir,
                                        const float srcZ,
                                        const float roomScale)
{
    auto &gl = getFakeGL();
    const glm::vec2 origin{0.f, 0.f};
    const float scale = sanitizeRoomScale(roomScale);

    switch (startDir) {
    case ExitDirEnum::NORTH:
        gl.drawTriangle(scalePointInRoom(glm::vec3{0.82f, 0.9f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.68f, 0.9f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.75f, 0.7f, srcZ}, origin, scale));
        break;
    case ExitDirEnum::SOUTH:
        gl.drawTriangle(scalePointInRoom(glm::vec3{0.18f, 0.1f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.32f, 0.1f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.25f, 0.3f, srcZ}, origin, scale));
        break;
    case ExitDirEnum::EAST:
        gl.drawTriangle(scalePointInRoom(glm::vec3{0.9f, 0.68f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.9f, 0.82f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.7f, 0.75f, srcZ}, origin, scale));
        break;
    case ExitDirEnum::WEST:
        gl.drawTriangle(scalePointInRoom(glm::vec3{0.1f, 0.32f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.1f, 0.18f, srcZ}, origin, scale),
                        scalePointInRoom(glm::vec3{0.3f, 0.25f, srcZ}, origin, scale));
        break;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        // Do not draw triangles for 2-way up/down
        break;
    case ExitDirEnum::UNKNOWN:
        drawConnEndTriUpDownUnknown(0, 0, srcZ, scale);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}

void ConnectionDrawer::drawConnEndTri(const ExitDirEnum endDir,
                                      const float dX,
                                      const float dY,
                                      const float dstZ,
                                      const float roomScale)
{
    auto &gl = getFakeGL();
    const glm::vec2 origin{dX, dY};
    const float scale = sanitizeRoomScale(roomScale);

    switch (endDir) {
    case ExitDirEnum::NORTH:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.82f, dY + 0.9f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.68f, dY + 0.9f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.75f, dY + 0.7f, dstZ}, origin, scale));
        break;
    case ExitDirEnum::SOUTH:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.18f, dY + 0.1f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.32f, dY + 0.1f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.25f, dY + 0.3f, dstZ}, origin, scale));
        break;
    case ExitDirEnum::EAST:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.9f, dY + 0.68f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.9f, dY + 0.82f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.7f, dY + 0.75f, dstZ}, origin, scale));
        break;
    case ExitDirEnum::WEST:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.1f, dY + 0.32f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.1f, dY + 0.18f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.3f, dY + 0.25f, dstZ}, origin, scale));
        break;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        // Do not draw triangles for 2-way up/down
        break;
    case ExitDirEnum::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUpDownUnknown(dX, dY, dstZ, scale);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}

void ConnectionDrawer::drawConnEndTri1Way(const ExitDirEnum endDir,
                                          const float dX,
                                          const float dY,
                                          const float dstZ,
                                          const float roomScale)
{
    auto &gl = getFakeGL();
    const glm::vec2 origin{dX, dY};
    const float scale = sanitizeRoomScale(roomScale);

    switch (endDir) {
    case ExitDirEnum::NORTH:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.32f, dY + 0.9f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.18f, dY + 0.9f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.25f, dY + 0.7f, dstZ}, origin, scale));
        break;
    case ExitDirEnum::SOUTH:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.68f, dY + 0.1f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.82f, dY + 0.1f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.75f, dY + 0.3f, dstZ}, origin, scale));
        break;
    case ExitDirEnum::EAST:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.9f, dY + 0.18f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.9f, dY + 0.32f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.7f, dY + 0.25f, dstZ}, origin, scale));
        break;
    case ExitDirEnum::WEST:
        gl.drawTriangle(scalePointInRoom(glm::vec3{dX + 0.1f, dY + 0.82f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.1f, dY + 0.68f, dstZ}, origin, scale),
                        scalePointInRoom(glm::vec3{dX + 0.3f, dY + 0.75f, dstZ}, origin, scale));
        break;

    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
    case ExitDirEnum::UNKNOWN:
        // NOTE: This is drawn for both 1-way and 2-way
        drawConnEndTriUpDownUnknown(dX, dY, dstZ, scale);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}

void ConnectionDrawer::drawConnEndTriUpDownUnknown(float dX,
                                                   float dY,
                                                   float dstZ,
                                                   const float roomScale)
{
    const glm::vec2 origin{dX, dY};
    const float scale = sanitizeRoomScale(roomScale);
    getFakeGL().drawTriangle(
        scalePointInRoom(glm::vec3{dX + 0.5f, dY + 0.5f, dstZ}, origin, scale),
        scalePointInRoom(glm::vec3{dX + 0.55f, dY + 0.3f, dstZ}, origin, scale),
        scalePointInRoom(glm::vec3{dX + 0.7f, dY + 0.45f, dstZ}, origin, scale));
}

ConnectionMeshes ConnectionDrawerBuffers::getMeshes(OpenGL &gl) const
{
    ConnectionMeshes result;
    result.normalTris = gl.createColoredTriBatch(normal.triVerts);
    result.redTris = gl.createColoredTriBatch(red.triVerts);
    result.normalQuads = gl.createColoredQuadBatch(normal.quadVerts);
    result.redQuads = gl.createColoredQuadBatch(red.quadVerts);
    return result;
}

void ConnectionMeshes::render(const int thisLayer, const int focusedLayer) const
{
    const auto color = std::invoke([&thisLayer, &focusedLayer]() -> Color {
        if (thisLayer == focusedLayer) {
            return XNamedColor{NamedColorEnum::CONNECTION_NORMAL}.getColor();
        }
        return Colors::gray70.withAlpha(FAINT_CONNECTION_ALPHA);
    });
    const auto common_style = GLRenderState().withBlend(BlendModeEnum::TRANSPARENCY).withColor(color);

    // Even though we can draw colored lines and tris,
    // the reason for having separate lines is so red will always be on top.
    // If you don't think that's important, you can combine the batches.

    normalTris.render(common_style);
    redTris.render(common_style);
    normalQuads.render(common_style);
    redQuads.render(common_style);
}

void MapCanvas::paintNearbyConnectionPoints()
{
    const bool isSelection = m_canvasMouseMode == CanvasMouseModeEnum::SELECT_CONNECTIONS;
    using CD = ConnectionSelection::ConnectionDescriptor;

    static const auto allExits = std::invoke([]() -> ExitDirFlags {
        ExitDirFlags tmp;
        for (const ExitDirEnum dir : ALL_EXITS7) {
            tmp |= dir;
        }
        return tmp;
    });

    std::vector<ColorVert> points;
    const auto addPoint = [isSelection, &points](const Coordinate &roomCoord,
                                                 const RoomHandle &room,
                                                 const ExitDirEnum dir,
                                                 const std::optional<CD> &optFirst) -> void {
        if (!isNESWUD(dir) && dir != ExitDirEnum::UNKNOWN) {
            return;
        }

        if (optFirst) {
            const CD &first = optFirst.value();
            const CD second{room, dir};
            if (isSelection ? !CD::isCompleteExisting(first, second)
                            : !CD::isCompleteNew(first, second)) {
                return;
            }
        }

        const auto transform = getRoomRenderTransform(room);
        const glm::vec3 worldPoint = roomCoord.to_vec3() + getConnectionOffset(room, dir);
        points.emplace_back(Colors::cyan, applyLocalspaceTransform(transform, worldPoint));
    };
    const auto addPoints =
        [this, isSelection, &addPoint](const std::optional<MouseSel> &sel,
                                       const std::optional<CD> &optFirst) -> void {
        if (!sel.has_value()) {
            return;
        }
        const auto mouse = sel->getCoordinate();
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const Coordinate roomCoord = mouse + Coordinate(dx, dy, 0);
                const auto room = m_data.findRoomHandle(roomCoord);
                if (!room) {
                    continue;
                }

                ExitDirFlags dirs = isSelection ? m_data.getExitDirections(roomCoord) : allExits;
                if (optFirst) {
                    dirs |= ExitDirEnum::UNKNOWN;
                }

                dirs.for_each([&addPoint, &optFirst, &roomCoord, &room](ExitDirEnum dir) -> void {
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
        const Coordinate &c = valid.room.getPosition();
        const glm::vec3 pos = c.to_vec3();
        const auto transform = getRoomRenderTransform(valid.room);
        const glm::vec3 worldPos = pos + getConnectionOffset(valid.room, valid.direction);
        points.emplace_back(Colors::cyan, applyLocalspaceTransform(transform, worldPos));

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
    const auto optPos2 = std::invoke([this, &sel]() -> std::optional<glm::vec3> {
        if (sel.isSecondValid()) {
            return getPosition(sel.getSecond());
        } else if (hasSel2()) {
            return getSel2().to_vec3();
        } else {
            return std::nullopt;
        }
    });

    if (!optPos2) {
        return;
    }

    const glm::vec3 &pos2 = optPos2.value();

    auto &gl = getOpenGL();
    const auto rs = GLRenderState().withColor(Colors::red);

    {
        std::vector<ColorVert> verts;
        mmgl::generateLineQuadsSafe(verts, pos1, pos2, CONNECTION_LINE_WIDTH, Colors::red);
        gl.renderColoredQuads(verts, rs);
    }

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
    const auto &color = isNormal() ? getCanvasNamedColorOptions().connectionNormalColor
                                   : Colors::red;
    auto &verts = deref(m_currentBuffer).triVerts;
    verts.emplace_back(color, applyTransform(a) + m_offset);
    verts.emplace_back(color, applyTransform(b) + m_offset);
    verts.emplace_back(color, applyTransform(c) + m_offset);
}

void ConnectionDrawer::ConnectionFakeGL::drawLineStrip(const std::vector<glm::vec3> &points)
{
    const auto transform = [this](const glm::vec3 &vert) {
        return applyTransform(vert) + m_offset;
    };
    const float extension = CONNECTION_LINE_WIDTH * 0.5f;

    // Helper lambda to generate a quad between two points with a specific color.
    auto generateQuad = [&](const glm::vec3 &p1, const glm::vec3 &p2, const Color quad_color) {
        auto &verts = deref(m_currentBuffer).quadVerts;

        const glm::vec3 segment = p2 - p1;
        if (mmgl::isNearZero(segment)) {
            mmgl::drawZeroLengthSquare(verts, p1, CONNECTION_LINE_WIDTH, quad_color);
            return;
        }

        const glm::vec3 dir = glm::normalize(segment);
        const glm::vec3 perp_normal_1 = mmgl::getPerpendicularNormal(dir);
        mmgl::generateLineQuad(verts, p1, p2, CONNECTION_LINE_WIDTH, quad_color, perp_normal_1);

        // If the line crosses different Z-layers, draw a second perpendicular quad to form a "cross" shape.
        if (isCrossingZAxis(p1, p2)) {
            const glm::vec3 perp_normal_2 = mmgl::getOrthogonalNormal(dir, perp_normal_1);
            mmgl::generateLineQuad(verts, p1, p2, CONNECTION_LINE_WIDTH, quad_color, perp_normal_2);
        }
    };

    const auto size = points.size();
    assert(size >= 2);

    const Color base_color = isNormal() ? getCanvasNamedColorOptions().connectionNormalColor
                                        : Colors::red;

    for (size_t i = 1; i < size; ++i) {
        const glm::vec3 start_orig = points[i - 1u];
        const glm::vec3 end_orig = points[i];

        const glm::vec3 start_v = transform(start_orig);
        const glm::vec3 end_v = transform(end_orig);

        Color current_segment_color = base_color;

        // Handle original zero-length segments first.
        const glm::vec3 segment = end_v - start_v;
        if (mmgl::isNearZero(segment)) {
            generateQuad(start_v, end_v, current_segment_color);
            continue;
        }

        // If the segment crosses the Z-axis, apply fading.
        if (isCrossingZAxis(start_v, end_v)) {
            current_segment_color = current_segment_color.withAlpha(FAINT_CONNECTION_ALPHA);
        }

        glm::vec3 quad_start_v = start_v;
        glm::vec3 quad_end_v = end_v;
        const glm::vec3 segment_dir = glm::normalize(segment);

        // Extend the first and last segments for better visual continuity.
        if (i == 1) {
            // First segment of the polyline.
            quad_start_v = start_v - segment_dir * extension;
        }
        if (i == size - 1) {
            // Last segment of the polyline.
            quad_end_v = end_v + segment_dir * extension;
        }

        // If it's not a long line, just draw a single quad.
        if (!isLongLine(quad_start_v, quad_end_v)) {
            generateQuad(quad_start_v, quad_end_v, current_segment_color);
            continue;
        }

        // It is a long line, apply fading.
        const float len = glm::length(quad_end_v - quad_start_v);
        const float faintCutoff = (len > 1e-6f) ? (LONG_LINE_HALFLEN / len) : 0.5f;

        const glm::vec3 mid1 = glm::mix(quad_start_v, quad_end_v, faintCutoff);
        const glm::vec3 mid2 = glm::mix(quad_start_v, quad_end_v, 1.f - faintCutoff);
        const Color faint_color = current_segment_color.withAlpha(FAINT_CONNECTION_ALPHA);

        generateQuad(quad_start_v, mid1, current_segment_color);
        generateQuad(mid1, mid2, faint_color);
        generateQuad(mid2, quad_end_v, current_segment_color);
    }
}
