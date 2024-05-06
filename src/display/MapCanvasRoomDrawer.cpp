// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MapCanvasRoomDrawer.h"

#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/AnsiColor.h"
#include "../global/Array.h"
#include "../global/Debug.h"
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
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "ConnectionLineBuilder.h"
#include "MapCanvasData.h"
#include "RoadIndex.h"
#include "mapcanvas.h" // hack, since we're now definining some of its symbols

#include <cassert>
#include <cstdlib>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QColor>
#include <QMessageLogContext>
#include <QtGui/qopengl.h>
#include <QtGui>

enum class NODISCARD StreamTypeEnum { OutFlow, InFlow };
enum class NODISCARD WallTypeEnum { SOLID, DOTTED, DOOR };
static constexpr const size_t NUM_WALL_TYPES = 3;
DEFINE_ENUM_COUNT(WallTypeEnum, NUM_WALL_TYPES)

#define LOOKUP_COLOR(X) (getConfig().colorSettings.X)

NODISCARD static bool isTransparent(const XNamedColor &namedColor)
{
    return !namedColor.isInitialized() || namedColor == LOOKUP_COLOR(TRANSPARENT);
}

NODISCARD static std::optional<Color> getColor(const XNamedColor &namedColor)
{
    if (isTransparent(namedColor))
        return std::nullopt;
    return namedColor.getColor();
}

enum class NODISCARD WallOrientationEnum { HORIZONTAL, VERTICAL };
NODISCARD static XNamedColor getWallNamedColorCommon(const ExitFlags &flags,
                                                     const WallOrientationEnum wallOrientation)
{
    const bool isVertical = wallOrientation == WallOrientationEnum::VERTICAL;

    // Vertical colors override the horizontal case
    // REVISIT: consider using the same set and just override the color
    // using the same order of flag testing as the horizontal case.
    //
    // In other words, eliminate this `if (isVertical)` block and just use
    //   return (isVertical ? LOOKUP_COLOR(VERTICAL_COLOR_CLIMB) : LOOKUP_COLOR(WALL_COLOR_CLIMB));
    // in the appropriate places in the following chained test for flags.
    if (isVertical) {
        if (flags.isClimb()) {
            // NOTE: This color is slightly darker than WALL_COLOR_CLIMB
            return LOOKUP_COLOR(VERTICAL_COLOR_CLIMB);
        }
        /*FALL-THRU*/
    }

    if (flags.isNoFlee()) {
        return LOOKUP_COLOR(WALL_COLOR_NO_FLEE);
    } else if (flags.isRandom()) {
        return LOOKUP_COLOR(WALL_COLOR_RANDOM);
    } else if (flags.isFall() || flags.isDamage()) {
        return LOOKUP_COLOR(WALL_COLOR_FALL_DAMAGE);
    } else if (flags.isSpecial()) {
        return LOOKUP_COLOR(WALL_COLOR_SPECIAL);
    } else if (flags.isClimb()) {
        return LOOKUP_COLOR(WALL_COLOR_CLIMB);
    } else if (flags.isGuarded()) {
        return LOOKUP_COLOR(WALL_COLOR_GUARDED);
    } else if (flags.isNoMatch()) {
        return LOOKUP_COLOR(WALL_COLOR_NO_MATCH);
    } else {
        return LOOKUP_COLOR(TRANSPARENT);
    }
}

NODISCARD static XNamedColor getWallNamedColor(const ExitFlags &flags)
{
    return getWallNamedColorCommon(flags, WallOrientationEnum::HORIZONTAL);
}

NODISCARD static XNamedColor getVerticalNamedColor(const ExitFlags &flags)
{
    return getWallNamedColorCommon(flags, WallOrientationEnum::VERTICAL);
}

// All layers
static void generateAllLayerMeshes(MapBatches &batches,
                                   OpenGL &gl,
                                   GLFont &font,
                                   const LayerToRooms &layerToRooms,
                                   const RoomIndex &roomIndex,
                                   const MapCanvasTextures &textures,
                                   const OptBounds &bounds);

void MapCanvasRoomDrawer::generateBatches(const LayerToRooms &layerToRooms,
                                          const RoomIndex &roomIndex,
                                          const OptBounds &bounds)
{
    m_batches.reset();   // dtor, if necessary
    m_batches.emplace(); // ctor

    generateAllLayerMeshes(m_batches.value(),
                           getOpenGL(),
                           getFont(),
                           layerToRooms,
                           roomIndex,
                           m_textures,
                           bounds);
}

struct NODISCARD TerrainAndTrail
{
    MMTexture *terrain = nullptr;
    MMTexture *trail = nullptr;
};

NODISCARD static TerrainAndTrail getRoomTerrainAndTrail(const MapCanvasTextures &textures,
                                                        const Room *const room)
{
    const auto roomTerrainType = room->getTerrainType();
    const RoadIndexMaskEnum roadIndex = getRoadIndex(*room);

    TerrainAndTrail result;
    result.terrain = (roomTerrainType == RoomTerrainEnum::ROAD)
                         ? textures.road[roadIndex]->getRaw()
                         : textures.terrain[roomTerrainType]->getRaw();

    if (roadIndex != RoadIndexMaskEnum::NONE && roomTerrainType != RoomTerrainEnum::ROAD) {
        result.trail = textures.trail[roadIndex]->getRaw();
    }
    return result;
}

struct NODISCARD IRoomVisitorCallbacks
{
    virtual ~IRoomVisitorCallbacks();

private:
    NODISCARD virtual bool virt_acceptRoom(const Room *) const = 0;

public:
    NODISCARD bool acceptRoom(const Room *const pRoom) const { return virt_acceptRoom(pRoom); }

private:
    // Rooms
    virtual void virt_visitTerrainTexture(const Room *, MMTexture *) = 0;
    virtual void virt_visitOverlayTexture(const Room *, MMTexture *) = 0;
    virtual void virt_visitNamedColorTint(const Room *, RoomTintEnum) = 0;

    // Walls
    virtual void virt_visitWall(const Room *, ExitDirEnum, XNamedColor, WallTypeEnum, bool isClimb)
        = 0;

    // Streams
    virtual void virt_visitStream(const Room *, ExitDirEnum, StreamTypeEnum) = 0;

public:
    // Rooms
    void visitTerrainTexture(const Room *const room, MMTexture *const tex)
    {
        virt_visitTerrainTexture(room, tex);
    }
    void visitOverlayTexture(const Room *const room, MMTexture *const tex)
    {
        virt_visitOverlayTexture(room, tex);
    }
    void visitNamedColorTint(const Room *const room, const RoomTintEnum tint)
    {
        virt_visitNamedColorTint(room, tint);
    }

    // Walls
    void visitWall(const Room *const room,
                   const ExitDirEnum dir,
                   const XNamedColor color,
                   const WallTypeEnum wallType,
                   const bool isClimb)
    {
        virt_visitWall(room, dir, color, wallType, isClimb);
    }

    // Streams
    void visitStream(const Room *const room, const ExitDirEnum dir, const StreamTypeEnum streamType)
    {
        virt_visitStream(room, dir, streamType);
    }
};

IRoomVisitorCallbacks::~IRoomVisitorCallbacks() = default;

static void visitRoom(const Room *const room,
                      const RoomIndex &roomIndex,
                      const MapCanvasTextures &textures,
                      IRoomVisitorCallbacks &callbacks)
{
    if (!callbacks.acceptRoom(room))
        return;

    // const auto &pos = room->getPosition();
    const bool isDark = room->getLightType() == RoomLightEnum::DARK;
    const bool hasNoSundeath = room->getSundeathType() == RoomSundeathEnum::NO_SUNDEATH;
    const bool notRideable = room->getRidableType() == RoomRidableEnum::NOT_RIDABLE;
    const auto terrainAndTrail = getRoomTerrainAndTrail(textures, room);
    const RoomMobFlags mf = room->getMobFlags();
    const RoomLoadFlags lf = room->getLoadFlags();

    // FIXME: This requires a map update.
    // TODO: make this a separate mesh.
    const bool needsUpdate = getConfig().canvas.showUpdated && !room->isUpToDate();

    callbacks.visitTerrainTexture(room, terrainAndTrail.terrain);

    if (auto trail = terrainAndTrail.trail)
        callbacks.visitOverlayTexture(room, trail);

    if (isDark)
        callbacks.visitNamedColorTint(room, RoomTintEnum::DARK);
    else if (hasNoSundeath)
        callbacks.visitNamedColorTint(room, RoomTintEnum::NO_SUNDEATH);

    mf.for_each([&room, &textures, &callbacks](const RoomMobFlagEnum flag) -> void {
        callbacks.visitOverlayTexture(room, textures.mob[flag]->getRaw());
    });

    lf.for_each([&room, &textures, &callbacks](const RoomLoadFlagEnum flag) -> void {
        callbacks.visitOverlayTexture(room, textures.load[flag]->getRaw());
    });

    if (notRideable) {
        callbacks.visitOverlayTexture(room, textures.no_ride->getRaw());
    }

    // FIXME: This requires a map update.
    if (needsUpdate) {
        callbacks.visitOverlayTexture(room, textures.update->getRaw());
    }

    const auto drawInFlow = [room, &roomIndex, &callbacks](const Exit &exit,
                                                           const ExitDirEnum &dir) -> void {
        // For each incoming connections
        for (const auto &targetId : exit.inRange()) {
            const SharedConstRoom &targetRoom = roomIndex[targetId];
            if (targetRoom == nullptr)
                continue;
            for (const ExitDirEnum targetDir : ALL_EXITS_NESWUD) {
                const Exit &targetExit = targetRoom->exit(targetDir);
                const ExitFlags &flags = targetExit.getExitFlags();
                if (flags.isFlow() && targetExit.containsOut(room->getId())) {
                    callbacks.visitStream(room, dir, StreamTypeEnum::InFlow);
                    return;
                }
            }
        }
    };

    // drawExit()

    // FIXME: This requires a map update.
    // REVISIT: The logic of drawNotMappedExits seems a bit wonky.
    const auto drawNotMappedExits = getConfig().canvas.drawNotMappedExits;
    for (const ExitDirEnum dir : ALL_EXITS_NESW) {
        const Exit &exit = room->exit(dir);
        const ExitFlags &flags = exit.getExitFlags();
        const auto isExit = flags.isExit();
        const auto isDoor = flags.isDoor();

        const bool isClimb = flags.isClimb();

        // FIXME: This requires a map update.
        // TODO: make "not mapped" exits a separate mesh;
        // except what should we do for the "else" case?
        if (isExit && drawNotMappedExits && exit.outIsEmpty()) { // zero outgoing connections
            callbacks.visitWall(room,
                                dir,
                                LOOKUP_COLOR(WALL_COLOR_NOT_MAPPED),
                                WallTypeEnum::DOTTED,
                                isClimb);
        } else {
            const auto namedColor = getWallNamedColor(flags);
            if (!isTransparent(namedColor)) {
                callbacks.visitWall(room, dir, namedColor, WallTypeEnum::DOTTED, isClimb);
            }

            if (flags.isFlow()) {
                callbacks.visitStream(room, dir, StreamTypeEnum::OutFlow);
            }
        }

        // wall
        if (!isExit || isDoor) {
            if (!isDoor && !exit.outIsEmpty()) {
                callbacks.visitWall(room,
                                    dir,
                                    LOOKUP_COLOR(WALL_COLOR_BUG_WALL_DOOR),
                                    WallTypeEnum::DOTTED,
                                    isClimb);
            } else {
                callbacks.visitWall(room,
                                    dir,
                                    LOOKUP_COLOR(WALL_COLOR_REGULAR_EXIT),
                                    WallTypeEnum::SOLID,
                                    isClimb);
            }
        }
        // door
        if (isDoor) {
            callbacks.visitWall(room,
                                dir,
                                LOOKUP_COLOR(WALL_COLOR_REGULAR_EXIT),
                                WallTypeEnum::DOOR,
                                isClimb);
        }

        if (!exit.inIsEmpty())
            drawInFlow(exit, dir);
    }

    // drawVertical
    for (const ExitDirEnum dir : {ExitDirEnum::UP, ExitDirEnum::DOWN}) {
        const Exit &exit = room->exit(dir);
        const auto &flags = exit.getExitFlags();
        if (!flags.isExit())
            continue;

        const bool isClimb = flags.isClimb();

        // FIXME: This requires a map update.
        if (drawNotMappedExits && exit.outIsEmpty()) { // zero outgoing connections
            callbacks.visitWall(room,
                                dir,
                                LOOKUP_COLOR(WALL_COLOR_NOT_MAPPED),
                                WallTypeEnum::DOTTED,
                                isClimb);
            continue;
        }

        // NOTE: in the "old" version, this falls-thru and the custom color is overwritten
        // by the regular exit; so using if-else here is a bug fix.
        const auto namedColor = getVerticalNamedColor(flags);
        if (!isTransparent(namedColor)) {
            callbacks.visitWall(room, dir, namedColor, WallTypeEnum::DOTTED, isClimb);
        } else {
            callbacks.visitWall(room,
                                dir,
                                LOOKUP_COLOR(VERTICAL_COLOR_REGULAR_EXIT),
                                WallTypeEnum::SOLID,
                                isClimb);
        }

        if (flags.isDoor()) {
            callbacks.visitWall(room,
                                dir,
                                LOOKUP_COLOR(WALL_COLOR_REGULAR_EXIT),
                                WallTypeEnum::DOOR,
                                isClimb);
        }

        if (flags.isFlow()) {
            callbacks.visitStream(room, dir, StreamTypeEnum::OutFlow);
        }

        if (!exit.inIsEmpty())
            drawInFlow(exit, dir);
    }
}

static void visitRooms(const RoomVector &rooms,
                       const RoomIndex &roomIndex,
                       const MapCanvasTextures &textures,
                       IRoomVisitorCallbacks &callbacks)
{
    for (const auto &room : rooms) {
        visitRoom(room, roomIndex, textures, callbacks);
    }
}

struct NODISCARD RoomTex
{
    const Room *room = nullptr;
    MMTexture *tex = nullptr;

    explicit RoomTex(const Room *const _room, MMTexture *const _tex)
        : room{_room}
        , tex{_tex}
    {
        deref(_tex);
    }

    NODISCARD int priority() const { return deref(tex).getPriority(); }
    NODISCARD GLuint textureId() const { return deref(tex).textureId(); }

    friend bool operator<(const RoomTex &lhs, const RoomTex &rhs)
    {
        // true if lhs comes strictly before rhs
        return lhs.priority() < rhs.priority();
    }
};

struct NODISCARD ColoredRoomTex : public RoomTex
{
    Color color;
    ColoredRoomTex(const Room *const room, MMTexture *const tex) = delete;

    explicit ColoredRoomTex(const Room *const _room, MMTexture *const _tex, const Color &_color)
        : RoomTex{_room, _tex}
        , color{_color}
    {
        deref(_tex);
    }
};

// Caution: Although O(n) partitioning into an array indexed by constant number of texture IDs
// is theoretically faster than O(n log n) sorting, one naive attempt to prematurely optimize
// this code resulted in a 50x slow-down.
//
// Note: sortByTexture() probably won't ever be a performance bottleneck for the default map,
// since at the time of this comment, the full O(n log n) vector sort only takes up about 2%
// of the total runtime of the mesh generation.
//
// Conclusion: Look elsewhere for optimization opportunities -- at least until profiling says
// that sorting is at significant fraction of the total runtime.
struct NODISCARD RoomTexVector final : public std::vector<RoomTex>
{
    // sorting stl iterators is slower than christmas with GLIBCXX_DEBUG,
    // so we'll use pointers instead. std::stable_sort isn't
    // necessary because everything's opaque, so we'll use
    // vanilla std::sort, which is N log N instead of N^2 log N.
    void sortByTexture()
    {
        if (size() < 2)
            return;

        RoomTex *const beg = data();
        RoomTex *const end = beg + size();
        // NOTE: comparison will be std::less<RoomTex>, which uses
        // operator<() if it exists.
        std::sort(beg, end);
    }

    NODISCARD bool isSorted() const
    {
        if (size() < 2)
            return true;

        const RoomTex *const beg = data();
        const RoomTex *const end = beg + size();
        return std::is_sorted(beg, end);
    }
};

struct NODISCARD ColoredRoomTexVector final : public std::vector<ColoredRoomTex>
{
    // sorting stl iterators is slower than christmas with GLIBCXX_DEBUG,
    // so we'll use pointers instead. std::stable_sort isn't
    // necessary because everything's opaque, so we'll use
    // vanilla std::sort, which is N log N instead of N^2 log N.
    void sortByTexture()
    {
        if (size() < 2)
            return;

        ColoredRoomTex *const beg = data();
        ColoredRoomTex *const end = beg + size();
        // NOTE: comparison will be std::less<RoomTex>, which uses
        // operator<() if it exists.
        std::sort(beg, end);
    }

    NODISCARD bool isSorted() const
    {
        if (size() < 2)
            return true;

        const ColoredRoomTex *const beg = data();
        const ColoredRoomTex *const end = beg + size();
        return std::is_sorted(beg, end);
    }
};

template<typename T, typename Callback>
static void foreach_texture(const T &textures, Callback &&callback)
{
    assert(textures.isSorted());

    const auto size = textures.size();
    for (size_t beg = 0, next = size; beg < size; beg = next) {
        const RoomTex &rtex = textures[beg];
        const auto textureId = rtex.textureId();

        size_t end = beg + 1;
        for (; end < size; ++end)
            if (textureId != textures[end].textureId())
                break;

        next = end;
        /* note: creating temporaries to prevent callback from modifying beg and end */
        callback(size_t(beg), size_t(end));
    }
}

NODISCARD static UniqueMeshVector createSortedTexturedMeshes(OpenGL &gl,
                                                             const RoomTexVector &textures)
{
    if (textures.empty())
        return UniqueMeshVector{};

    const size_t numUniqueTextures = [&textures]() -> size_t {
        size_t texCount = 0;
        ::foreach_texture(textures,
                          [&texCount](size_t /* beg */, size_t /*end*/) -> void { ++texCount; });
        return texCount;
    }();

    std::vector<UniqueMesh> result_meshes;
    result_meshes.reserve(numUniqueTextures);
    const auto lambda = [&gl, &result_meshes, &textures](const size_t beg,
                                                         const size_t end) -> void {
        const RoomTex &rtex = textures[beg];
        const size_t count = end - beg;

        std::vector<TexVert> verts;
        verts.reserve(count * VERTS_PER_QUAD); /* quads */

        // D-C
        // | |  ccw winding
        // A-B
        for (size_t i = beg; i < end; ++i) {
            const auto &pos = textures[i].room->getPosition();
            const auto v0 = pos.to_vec3();
#define EMIT(x, y) verts.emplace_back(glm::vec2((x), (y)), v0 + glm::vec3((x), (y), 0))
            EMIT(0, 0);
            EMIT(1, 0);
            EMIT(1, 1);
            EMIT(0, 1);
#undef EMIT
        }

        result_meshes.emplace_back(gl.createTexturedQuadBatch(verts, rtex.tex->getShared()));
    };

    ::foreach_texture(textures, lambda);
    assert(result_meshes.size() == numUniqueTextures);
    return UniqueMeshVector{std::move(result_meshes)};
}

NODISCARD static UniqueMeshVector createSortedColoredTexturedMeshes(
    OpenGL &gl, const ColoredRoomTexVector &textures)
{
    if (textures.empty())
        return UniqueMeshVector{};

    const size_t numUniqueTextures = [&textures]() -> size_t {
        size_t texCount = 0;
        ::foreach_texture(textures,
                          [&texCount](size_t /* beg */, size_t /*end*/) -> void { ++texCount; });
        return texCount;
    }();

    std::vector<UniqueMesh> result_meshes;
    result_meshes.reserve(numUniqueTextures);

    const auto lambda = [&gl, &result_meshes, &textures](const size_t beg,
                                                         const size_t end) -> void {
        const RoomTex &rtex = textures[beg];
        const size_t count = end - beg;

        std::vector<ColoredTexVert> verts;
        verts.reserve(count * VERTS_PER_QUAD); /* quads */

        // D-C
        // | |  ccw winding
        // A-B
        for (size_t i = beg; i < end; ++i) {
            const ColoredRoomTex &thisVert = textures[i];
            const auto &pos = thisVert.room->getPosition();
            const auto v0 = pos.to_vec3();
            const auto color = thisVert.color;

#define EMIT(x, y) verts.emplace_back(color, glm::vec2((x), (y)), v0 + glm::vec3((x), (y), 0))
            EMIT(0, 0);
            EMIT(1, 0);
            EMIT(1, 1);
            EMIT(0, 1);
#undef EMIT
        }

        result_meshes.emplace_back(gl.createColoredTexturedQuadBatch(verts, rtex.tex->getShared()));
    };

    ::foreach_texture(textures, lambda);
    assert(result_meshes.size() == numUniqueTextures);
    return UniqueMeshVector{std::move(result_meshes)};
}

using PlainQuadBatch = std::vector<glm::vec3>;

struct NODISCARD LayerBatchData final
{
    RoomTexVector roomTerrains;
    RoomTexVector roomOverlays;
    // REVISIT: Consider storing up/down door lines in a separate batch,
    // so they can be rendered thicker.
    ColoredRoomTexVector doors;
    ColoredRoomTexVector solidWallLines;
    ColoredRoomTexVector dottedWallLines;
    ColoredRoomTexVector roomUpDownExits;
    ColoredRoomTexVector streamIns;
    ColoredRoomTexVector streamOuts;
    RoomTintArray<PlainQuadBatch> roomTints;
    PlainQuadBatch roomLayerBoostQuads;

    explicit LayerBatchData() = default;
    ~LayerBatchData() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(LayerBatchData);

    void sort()
    {
        /* TODO: Only sort on 2.1 path, since 3.0 can use GL_TEXTURE_2D_ARRAY. */
        roomTerrains.sortByTexture();
        roomOverlays.sortByTexture();

        // REVISIT: We could just make two separate lists and avoid the sort.
        // However, it may be convenient to have separate dotted vs solid texture,
        // so we'd still need to sort in that case.
        roomUpDownExits.sortByTexture();
        doors.sortByTexture();
        solidWallLines.sortByTexture();
        dottedWallLines.sortByTexture();
        streamIns.sortByTexture();
        streamOuts.sortByTexture();
    }

    NODISCARD LayerMeshes getMeshes(OpenGL &gl)
    {
        LayerMeshes meshes;
        meshes.terrain = ::createSortedTexturedMeshes(gl, roomTerrains);
        for (const RoomTintEnum tint : ALL_ROOM_TINTS) {
            meshes.tints[tint] = gl.createPlainQuadBatch(roomTints[tint]);
        }
        meshes.overlays = ::createSortedTexturedMeshes(gl, roomOverlays);
        meshes.doors = ::createSortedColoredTexturedMeshes(gl, doors);
        meshes.walls = ::createSortedColoredTexturedMeshes(gl, solidWallLines);
        meshes.dottedWalls = ::createSortedColoredTexturedMeshes(gl, dottedWallLines);
        meshes.upDownExits = ::createSortedColoredTexturedMeshes(gl, roomUpDownExits);
        meshes.streamIns = ::createSortedColoredTexturedMeshes(gl, streamIns);
        meshes.streamOuts = ::createSortedColoredTexturedMeshes(gl, streamOuts);
        meshes.layerBoost = gl.createPlainQuadBatch(roomLayerBoostQuads);
        meshes.isValid = true;
        return meshes;
    }
};

class NODISCARD LayerBatchBuilder final : public IRoomVisitorCallbacks
{
private:
    LayerBatchData &m_data;
    const MapCanvasTextures &m_textures;
    const OptBounds &m_bounds;

public:
    explicit LayerBatchBuilder(LayerBatchData &data,
                               const MapCanvasTextures &textures,
                               const OptBounds &bounds)
        : m_data{data}
        , m_textures{textures}
        , m_bounds{bounds}
    {}

    ~LayerBatchBuilder() override;

    DELETE_CTORS_AND_ASSIGN_OPS(LayerBatchBuilder);

    NODISCARD bool virt_acceptRoom(const Room *const room) const override
    {
        return m_bounds.contains(room->getPosition());
    }

private:
    void virt_visitTerrainTexture(const Room *const room, MMTexture *const terrain) final
    {
        if (terrain == nullptr)
            return;

        m_data.roomTerrains.emplace_back(room, terrain);

        const auto v0 = room->getPosition().to_vec3();
#define EMIT(x, y) m_data.roomLayerBoostQuads.emplace_back(v0 + glm::vec3((x), (y), 0))
        EMIT(0, 0);
        EMIT(1, 0);
        EMIT(1, 1);
        EMIT(0, 1);
#undef EMIT
    }

    void virt_visitOverlayTexture(const Room *const room, MMTexture *const overlay) final
    {
        if (overlay != nullptr)
            m_data.roomOverlays.emplace_back(room, overlay);
    }

    void virt_visitNamedColorTint(const Room *const room, const RoomTintEnum tint) final
    {
        const auto v0 = room->getPosition().to_vec3();
#define EMIT(x, y) m_data.roomTints[tint].emplace_back(v0 + glm::vec3((x), (y), 0))
        EMIT(0, 0);
        EMIT(1, 0);
        EMIT(1, 1);
        EMIT(0, 1);
#undef EMIT
    }

    void virt_visitWall(const Room *const room,
                        const ExitDirEnum dir,
                        const XNamedColor color,
                        const WallTypeEnum wallType,
                        const bool isClimb) final
    {
        if (isTransparent(color))
            return;

        const std::optional<Color> optColor = getColor(color);
        if (!optColor.has_value()) {
            assert(false);
            return;
        }

        const auto glcolor = optColor.value();

        if (wallType == WallTypeEnum::DOOR) {
            // Note: We could use two door textures (NESW and UD), and then just rotate the
            // texture coordinates, but doing that would require a different code path.
            const SharedMMTexture &tex = m_textures.door[dir];
            m_data.doors.emplace_back(room, tex->getRaw(), glcolor);

        } else {
            if (isNESW(dir)) {
                if (wallType == WallTypeEnum::SOLID) {
                    const SharedMMTexture &tex = m_textures.wall[dir];
                    m_data.solidWallLines.emplace_back(room, tex->getRaw(), glcolor);
                } else {
                    const SharedMMTexture &tex = m_textures.dotted_wall[dir];
                    m_data.dottedWallLines.emplace_back(room, tex->getRaw(), glcolor);
                }
            } else {
                const bool isUp = dir == ExitDirEnum::UP;
                assert(isUp || dir == ExitDirEnum::DOWN);

                const SharedMMTexture &tex = isClimb ? (isUp ? m_textures.exit_climb_up
                                                             : m_textures.exit_climb_down)
                                                     : (isUp ? m_textures.exit_up
                                                             : m_textures.exit_down);

                m_data.roomUpDownExits.emplace_back(room, tex->getRaw(), glcolor);
            }
        }
    }

    void virt_visitStream(const Room *const room,
                          const ExitDirEnum dir,
                          const StreamTypeEnum type) final
    {
        const Color color = LOOKUP_COLOR(STREAM).getColor();
        switch (type) {
        case StreamTypeEnum::OutFlow:
            m_data.streamOuts.emplace_back(room, m_textures.stream_out[dir]->getRaw(), color);
            return;
        case StreamTypeEnum::InFlow:
            m_data.streamIns.emplace_back(room, m_textures.stream_in[dir]->getRaw(), color);
            return;
        default:
            break;
        }

        assert(false);
    }
};

LayerBatchBuilder::~LayerBatchBuilder() = default;

NODISCARD static LayerMeshes generateLayerMeshes(OpenGL &gl,
                                                 const RoomVector &rooms,
                                                 const RoomIndex &roomIndex,
                                                 const MapCanvasTextures &textures,
                                                 const OptBounds &bounds)
{
    LayerBatchData data;
    LayerBatchBuilder builder{data, textures, bounds};
    visitRooms(rooms, roomIndex, textures, builder);
    data.sort();
    return data.getMeshes(gl);
}

static void generateAllLayerMeshes(MapBatches &batches,
                                   OpenGL &gl,
                                   GLFont &font,
                                   const LayerToRooms &layerToRooms,
                                   const RoomIndex &roomIndex,
                                   const MapCanvasTextures &textures,
                                   const OptBounds &bounds)
{
    auto &batchedMeshes = batches.batchedMeshes;
    auto &connectionDrawerBuffers = batches.connectionDrawerBuffers;
    auto &roomNameBatches = batches.roomNameBatches;
    auto &connectionMeshes = batches.connectionMeshes;

    for (const auto &layer : layerToRooms) {
        const int thisLayer = layer.first;
        auto &layerMeshes = batchedMeshes[thisLayer];
        auto &layerRoomNames = roomNameBatches[thisLayer];
        auto &layerConnections = connectionMeshes[thisLayer];
        auto &rooms = layer.second;

        layerMeshes = ::generateLayerMeshes(gl, rooms, roomIndex, textures, bounds);

        {
            auto &cdb = connectionDrawerBuffers[thisLayer];
            cdb.clear();

            const auto currentLayer = thisLayer;

            RoomNameBatch rnb;
            ConnectionDrawer cd{cdb, rnb, currentLayer, bounds};
            for (const auto &room : rooms) {
                cd.drawRoomConnectionsAndDoors(room, roomIndex);
            }

            layerRoomNames = rnb.getMesh(font);
            layerConnections = cdb.getMeshes(gl);
        }
    }
}

void LayerMeshes::render(const int thisLayer, const int focusedLayer)
{
    bool disableTextures = false;
    if (thisLayer > focusedLayer) {
        if (!getConfig().canvas.drawUpperLayersTextured) {
            // Disable texturing for this layer. We want to draw
            // all of the squares in white (using layer boost quads),
            // and then still draw the walls.
            disableTextures = true;
        }
    }

    const GLRenderState less = GLRenderState().withDepthFunction(DepthFunctionEnum::LESS);
    const GLRenderState equal = GLRenderState().withDepthFunction(DepthFunctionEnum::EQUAL);
    const GLRenderState lequal = GLRenderState().withDepthFunction(DepthFunctionEnum::LEQUAL);

    const GLRenderState less_blended = less.withBlend(BlendModeEnum::TRANSPARENCY);
    const GLRenderState lequal_blended = lequal.withBlend(BlendModeEnum::TRANSPARENCY);
    const GLRenderState equal_blended = equal.withBlend(BlendModeEnum::TRANSPARENCY);
    const GLRenderState equal_multiplied = equal.withBlend(BlendModeEnum::MODULATE);

    const auto color = [&thisLayer, &focusedLayer]() {
        if (thisLayer <= focusedLayer) {
            return Colors::white.withAlpha(0.90f);
        }
        return Colors::gray70.withAlpha(0.20f);
    }();

    {
        /* REVISIT: For the modern case, we could render each layer separately,
         * and then only blend the layers that actually overlap. Doing that would
         * give higher contrast for the base textures.
         */
        if (disableTextures) {
            const auto layerWhite = Colors::white.withAlpha(0.20f);
            layerBoost.render(less_blended.withColor(layerWhite));
        } else {
            terrain.render(less_blended.withColor(color));
        }
    }

    // REVISIT: move trails to their own batch also colored by the tint?
    for (const RoomTintEnum tint : ALL_ROOM_TINTS) {
        static_assert(NUM_ROOM_TINTS == 2);
        const auto namedColor = [tint]() -> XNamedColor {
            switch (tint) {
            case RoomTintEnum::DARK:
                return LOOKUP_COLOR(ROOM_DARK);
            case RoomTintEnum::NO_SUNDEATH:
                return LOOKUP_COLOR(ROOM_NO_SUNDEATH);
            }
            std::abort();
        }();

        if (const auto optColor = getColor(namedColor)) {
            tints[tint].render(equal_multiplied.withColor(optColor.value()));
        } else {
            assert(false);
        }
    }

    if (!disableTextures) {
        // streams go under everything else, including trails
        streamIns.render(lequal_blended.withColor(color));
        streamOuts.render(lequal_blended.withColor(color));

        overlays.render(equal_blended.withColor(color));
    }

    // always
    {
        // doors and walls are considered lines, even though they're drawn with textures.
        upDownExits.render(equal_blended.withColor(color));

        // Doors are drawn on top of the up-down exits
        doors.render(lequal_blended.withColor(color));
        // and walls are drawn on top of doors.
        walls.render(lequal_blended.withColor(color));
        dottedWalls.render(lequal_blended.withColor(color));
    }

    if (thisLayer != focusedLayer) {
        // Darker when below, lighter when above
        const auto baseAlpha = (thisLayer < focusedLayer) ? 0.5f : 0.1f;
        const auto alpha
            = glm::clamp(baseAlpha + 0.03f * static_cast<float>(std::abs(focusedLayer - thisLayer)),
                         0.f,
                         1.f);
        const Color &baseColor = (thisLayer < focusedLayer || disableTextures) ? Colors::black
                                                                               : Colors::white;
        layerBoost.render(equal_blended.withColor(baseColor.withAlpha(alpha)));
    }
}
