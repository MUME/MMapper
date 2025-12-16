// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MapCanvasRoomDrawer.h"

#include "../clock/mumemoment.h"
#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../global/Array.h"
#include "../global/ConfigConsts.h"
#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"
#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "../map/DoorFlags.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/Map.h"
#include "../map/coordinate.h"
#include "../map/enums.h"
#include "../map/exit.h"
#include "../map/infomark.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../mapdata/mapdata.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "ConnectionLineBuilder.h"
#include "MapCanvasData.h"
#include "RoadIndex.h"
#include "mapcanvas.h" // hack, since we're now definining some of its symbols

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QColor>
#include <QMessageLogContext>
#include <QtGui/qopengl.h>
#include <QtGui>

struct NODISCARD VisitRoomOptions final
{
    SharedCanvasNamedColorOptions canvasColors;
    SharedNamedColorOptions colorSettings;
    bool drawNotMappedExits = false;
};

enum class NODISCARD StreamTypeEnum { OutFlow, InFlow };
enum class NODISCARD WallTypeEnum { SOLID, DOTTED, DOOR };
static constexpr const size_t NUM_WALL_TYPES = 3;
DEFINE_ENUM_COUNT(WallTypeEnum, NUM_WALL_TYPES)

#define LOOKUP_COLOR(X) (getNamedColorOptions().X)

// must be called from the main thread
NODISCARD static VisitRoomOptions getVisitRoomOptions()
{
    const auto &config = getConfig();
    auto &canvas = config.canvas;
    VisitRoomOptions result;
    result.canvasColors = canvas.clone();
    result.colorSettings = config.colorSettings.clone();
    result.drawNotMappedExits = canvas.showUnmappedExits.get();
    return result;
}

NODISCARD static bool isTransparent(const XNamedColor &namedColor)
{
    return !namedColor.isInitialized() || namedColor == LOOKUP_COLOR(TRANSPARENT);
}

NODISCARD static std::optional<Color> getColor(const XNamedColor &namedColor)
{
    if (isTransparent(namedColor)) {
        return std::nullopt;
    }
    return namedColor.getColor();
}

enum class NODISCARD WallOrientationEnum { HORIZONTAL, VERTICAL };
NODISCARD static XNamedColor getWallNamedColorCommon(const ExitFlags flags,
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

NODISCARD static XNamedColor getWallNamedColor(const ExitFlags flags)
{
    return getWallNamedColorCommon(flags, WallOrientationEnum::HORIZONTAL);
}

NODISCARD static XNamedColor getVerticalNamedColor(const ExitFlags flags)
{
    return getWallNamedColorCommon(flags, WallOrientationEnum::VERTICAL);
}

struct LayerBatchData;

struct NODISCARD TerrainAndTrail final
{
    MMTextureId terrain = INVALID_MM_TEXTURE_ID;
    MMTextureId trail = INVALID_MM_TEXTURE_ID;
};

NODISCARD static TerrainAndTrail getRoomTerrainAndTrail(const mctp::MapCanvasTexturesProxy &textures,
                                                        const RawRoom &room)
{
    const auto roomTerrainType = room.getTerrainType();
    const RoadIndexMaskEnum roadIndex = getRoadIndex(room);

    TerrainAndTrail result;
    result.terrain = (roomTerrainType == RoomTerrainEnum::ROAD) ? textures.road[roadIndex]
                                                                : textures.terrain[roomTerrainType];

    if (roadIndex != RoadIndexMaskEnum::NONE && roomTerrainType != RoomTerrainEnum::ROAD) {
        result.trail = textures.trail[roadIndex];
    }
    return result;
}

struct NODISCARD IRoomVisitorCallbacks
{
    virtual ~IRoomVisitorCallbacks();

private:
    NODISCARD virtual bool virt_acceptRoom(const RoomHandle &) const = 0;

private:
    // Rooms
    virtual void virt_visitTerrainTexture(const RoomHandle &, MMTextureId) = 0;
    virtual void virt_visitTrailTexture(const RoomHandle &, MMTextureId) = 0;
    virtual void virt_visitOverlayTexture(const RoomHandle &, MMTextureId) = 0;
    virtual void virt_visitNamedColorTint(const RoomHandle &, RoomTintEnum) = 0;

    // Walls
    virtual void virt_visitWall(
        const RoomHandle &, ExitDirEnum, const XNamedColor &, WallTypeEnum, bool isClimb)
        = 0;

    // Streams
    virtual void virt_visitStream(const RoomHandle &, ExitDirEnum, StreamTypeEnum) = 0;

public:
    NODISCARD bool acceptRoom(const RoomHandle &room) const { return virt_acceptRoom(room); }

    // Rooms
    void visitTerrainTexture(const RoomHandle &room, const MMTextureId tex)
    {
        virt_visitTerrainTexture(room, tex);
    }
    void visitTrailTexture(const RoomHandle &room, const MMTextureId tex)
    {
        virt_visitTrailTexture(room, tex);
    }
    void visitOverlayTexture(const RoomHandle &room, const MMTextureId tex)
    {
        virt_visitOverlayTexture(room, tex);
    }
    void visitNamedColorTint(const RoomHandle &room, const RoomTintEnum tint)
    {
        virt_visitNamedColorTint(room, tint);
    }

    // Walls
    void visitWall(const RoomHandle &room,
                   const ExitDirEnum dir,
                   const XNamedColor &color,
                   const WallTypeEnum wallType,
                   const bool isClimb)
    {
        virt_visitWall(room, dir, color, wallType, isClimb);
    }

    // Streams
    void visitStream(const RoomHandle &room, const ExitDirEnum dir, StreamTypeEnum streamType)
    {
        virt_visitStream(room, dir, streamType);
    }
};

IRoomVisitorCallbacks::~IRoomVisitorCallbacks() = default;

static void visitRoom(const RoomHandle &room,
                      const mctp::MapCanvasTexturesProxy &textures,
                      IRoomVisitorCallbacks &callbacks,
                      const VisitRoomOptions &visitRoomOptions,
                      const MumeTimeEnum timeOfDay,
                      const std::optional<RoomId> playerRoom,
                      const bool playerHasLight)
{
    if (!callbacks.acceptRoom(room)) {
        return;
    }

    // Check if this is the player's current room
    const bool isPlayerRoom = playerRoom.has_value() && room.getId() == playerRoom.value();

    // const auto &pos = room.getPosition();
    const RoomLightEnum roomLight = room.getLightType();
    const RoomTerrainEnum terrain = room.getTerrainType();
    const bool hasNoSundeath = room.getSundeathType() == RoomSundeathEnum::NO_SUNDEATH;
    const bool notRideable = room.getRidableType() == RoomRidableEnum::NOT_RIDABLE;
    const auto terrainAndTrail = getRoomTerrainAndTrail(textures, room.getRaw());
    const RoomMobFlags mf = room.getMobFlags();
    const RoomLoadFlags lf = room.getLoadFlags();

    callbacks.visitTerrainTexture(room, terrainAndTrail.terrain);

    if (auto trail = terrainAndTrail.trail) {
        callbacks.visitOverlayTexture(room, trail);
    }

    // Dynamic lighting based on time of day, room properties, terrain, and player's light
    // DEFAULT: All rooms are LIT unless explicitly dark or affected by conditions below
    bool isDark = false;

    // PRIORITY 0: Player's room with light source - ALWAYS LIT
    if (isPlayerRoom && playerHasLight) {
        isDark = false;
    }
    // Priority 1: Explicit DARK status (highest priority for non-player rooms - always dark)
    else if (roomLight == RoomLightEnum::DARK) {
        isDark = true;
    }
    // Priority 2: Explicit LIT status (overrides time/terrain - always lit, e.g., cities at night)
    else if (roomLight == RoomLightEnum::LIT) {
        isDark = false;
    }
    // Priority 3: UNDEFINED rooms - apply dynamic lighting rules
    else {
        // Rule 1: Always-dark terrains (underground areas for trolls)
        if (terrain == RoomTerrainEnum::CAVERN || terrain == RoomTerrainEnum::TUNNEL) {
            isDark = true;
        }
        // Rule 2: Time-based lighting for outdoor areas only
        else if ((timeOfDay == MumeTimeEnum::NIGHT || timeOfDay == MumeTimeEnum::DUSK)
                 && terrain != RoomTerrainEnum::INDOORS) {
            // Only outdoor (non-INDOORS) UNDEFINED rooms become dark at night/dusk
            isDark = true;
        }
        // Rule 3: Default for all other UNDEFINED rooms: LIT
        // (day time, indoors, or any terrain during day)
    }

    if (isDark) {
        callbacks.visitNamedColorTint(room, RoomTintEnum::DARK);
    } else if (hasNoSundeath) {
        callbacks.visitNamedColorTint(room, RoomTintEnum::NO_SUNDEATH);
    }

    mf.for_each([&room, &textures, &callbacks](const RoomMobFlagEnum flag) -> void {
        callbacks.visitOverlayTexture(room, textures.mob[flag]);
    });

    lf.for_each([&room, &textures, &callbacks](const RoomLoadFlagEnum flag) -> void {
        callbacks.visitOverlayTexture(room, textures.load[flag]);
    });

    if (notRideable) {
        callbacks.visitOverlayTexture(room, textures.no_ride);
    }

    const Map &map = room.getMap();
    const auto drawInFlow = [&map, &room, &callbacks](const RawExit &exit,
                                                      const ExitDirEnum &dir) -> void {
        // For each incoming connections
        for (const RoomId targetRoomId : exit.getIncomingSet()) {
            const auto &targetRoom = map.getRoomHandle(targetRoomId);
            for (const ExitDirEnum targetDir : ALL_EXITS_NESWUD) {
                const auto &targetExit = targetRoom.getExit(targetDir);
                const ExitFlags flags = targetExit.getExitFlags();
                if (flags.isFlow() && targetExit.containsOut(room.getId())) {
                    callbacks.visitStream(room, dir, StreamTypeEnum::InFlow);
                    return;
                }
            }
        }
    };

    // drawExit()

    // FIXME: This requires a map update.
    // REVISIT: The logic of drawNotMappedExits seems a bit wonky.
    for (const ExitDirEnum dir : ALL_EXITS_NESW) {
        const auto &exit = room.getExit(dir);
        const ExitFlags flags = exit.getExitFlags();
        const auto isExit = flags.isExit();
        const auto isDoor = flags.isDoor();

        const bool isClimb = flags.isClimb();

        // FIXME: This requires a map update.
        // TODO: make "not mapped" exits a separate mesh;
        // except what should we do for the "else" case?
        if (visitRoomOptions.drawNotMappedExits && exit.exitIsUnmapped()) {
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

        if (!exit.inIsEmpty()) {
            drawInFlow(exit, dir);
        }
    }

    // drawVertical
    for (const ExitDirEnum dir : {ExitDirEnum::UP, ExitDirEnum::DOWN}) {
        const auto &exit = room.getExit(dir);
        const auto &flags = exit.getExitFlags();
        const bool isClimb = flags.isClimb();

        if (visitRoomOptions.drawNotMappedExits && flags.isUnmapped()) {
            callbacks.visitWall(room,
                                dir,
                                LOOKUP_COLOR(WALL_COLOR_NOT_MAPPED),
                                WallTypeEnum::DOTTED,
                                isClimb);
            continue;
        } else if (!flags.isExit()) {
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

        if (!exit.inIsEmpty()) {
            drawInFlow(exit, dir);
        }
    }
}

static void visitRooms(const RoomVector &rooms,
                       const mctp::MapCanvasTexturesProxy &textures,
                       IRoomVisitorCallbacks &callbacks,
                       const VisitRoomOptions &visitRoomOptions,
                       const MumeTimeEnum timeOfDay,
                       const std::optional<RoomId> playerRoom,
                       const bool playerHasLight)
{
    DECL_TIMER(t, __FUNCTION__);
    for (const auto &room : rooms) {
        visitRoom(room, textures, callbacks, visitRoomOptions, timeOfDay, playerRoom, playerHasLight);
    }
}

struct NODISCARD RoomTex
{
    RoomHandle room;
    MMTextureId tex = INVALID_MM_TEXTURE_ID;

    explicit RoomTex(RoomHandle moved_room, const MMTextureId input_texid)
        : room{std::move(moved_room)}
        , tex{input_texid}
    {
        if (input_texid == INVALID_MM_TEXTURE_ID) {
            throw std::invalid_argument("input_texid");
        }
    }

    NODISCARD MMTextureId priority() const { return tex; }
    NODISCARD MMTextureId textureId() const { return tex; }

    NODISCARD friend bool operator<(const RoomTex &lhs, const RoomTex &rhs)
    {
        // true if lhs comes strictly before rhs
        return lhs.priority() < rhs.priority();
    }
};

struct NODISCARD ColoredRoomTex : public RoomTex
{
    Color color;
    ColoredRoomTex(const RoomHandle &room, const MMTextureId tex) = delete;

    explicit ColoredRoomTex(RoomHandle moved_room,
                            const MMTextureId input_texid,
                            const Color &input_color)
        : RoomTex{std::move(moved_room), input_texid}
        , color{input_color}
    {}
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
        if (size() < 2) {
            return;
        }

        RoomTex *const beg = data();
        RoomTex *const end = beg + size();
        // NOTE: comparison will be std::less<RoomTex>, which uses
        // operator<() if it exists.
        std::sort(beg, end);
    }

    NODISCARD bool isSorted() const
    {
        if (size() < 2) {
            return true;
        }

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
        if (size() < 2) {
            return;
        }

        ColoredRoomTex *const beg = data();
        ColoredRoomTex *const end = beg + size();
        // NOTE: comparison will be std::less<RoomTex>, which uses
        // operator<() if it exists.
        std::sort(beg, end);
    }

    NODISCARD bool isSorted() const
    {
        if (size() < 2) {
            return true;
        }

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
        for (; end < size; ++end) {
            if (textureId != textures[end].textureId()) {
                break;
            }
        }

        next = end;
        /* note: casting to avoid passing references to beg and end */
        callback(static_cast<size_t>(beg), static_cast<size_t>(end));
    }
}

NODISCARD static LayerMeshesIntermediate::FnVec createSortedTexturedMeshes(
    const std::string_view /*what*/, const RoomTexVector &textures)
{
    if (textures.empty()) {
        return {};
    }

    const size_t numUniqueTextures = [&textures]() -> size_t {
        size_t texCount = 0;
        ::foreach_texture(textures,
                          [&texCount](size_t /* beg */, size_t /*end*/) -> void { ++texCount; });
        return texCount;
    }();

    LayerMeshesIntermediate::FnVec tmp_meshes;
    tmp_meshes.reserve(numUniqueTextures);
    const auto lambda = [&tmp_meshes, &textures](const size_t beg, const size_t end) -> void {
        const RoomTex &rtex = textures[beg];
        const size_t count = end - beg;

        std::vector<TexVert> verts;
        verts.reserve(count * VERTS_PER_QUAD); /* quads */

        // D-C
        // | |  ccw winding
        // A-B
        for (size_t i = beg; i < end; ++i) {
            const auto &pos = textures[i].room.getPosition();
            const auto v0 = pos.to_vec3();
#define EMIT(x, y) verts.emplace_back(glm::vec2((x), (y)), v0 + glm::vec3((x), (y), 0))
            EMIT(0, 0);
            EMIT(1, 0);
            EMIT(1, 1);
            EMIT(0, 1);
#undef EMIT
        }

        tmp_meshes.emplace_back([v = std::move(verts), t = rtex.tex](OpenGL &g) {
            return g.createTexturedQuadBatch(v, t);
        });
    };

    ::foreach_texture(textures, lambda);
    assert(tmp_meshes.size() == numUniqueTextures);
    return tmp_meshes;
}

NODISCARD static LayerMeshesIntermediate::FnVec createSortedColoredTexturedMeshes(
    const std::string_view /*what*/, const ColoredRoomTexVector &textures)
{
    if (textures.empty()) {
        return {};
    }

    const size_t numUniqueTextures = [&textures]() -> size_t {
        size_t texCount = 0;
        ::foreach_texture(textures,
                          [&texCount](size_t /* beg */, size_t /*end*/) -> void { ++texCount; });
        return texCount;
    }();

    LayerMeshesIntermediate::FnVec tmp_meshes;
    tmp_meshes.reserve(numUniqueTextures);

    const auto lambda = [&tmp_meshes, &textures](const size_t beg, const size_t end) -> void {
        const RoomTex &rtex = textures[beg];
        const size_t count = end - beg;

        std::vector<ColoredTexVert> verts;
        verts.reserve(count * VERTS_PER_QUAD); /* quads */

        // D-C
        // | |  ccw winding
        // A-B
        for (size_t i = beg; i < end; ++i) {
            const ColoredRoomTex &thisVert = textures[i];
            const auto &pos = thisVert.room.getPosition();
            const auto v0 = pos.to_vec3();
            const auto color = thisVert.color;

#define EMIT(x, y) verts.emplace_back(color, glm::vec2((x), (y)), v0 + glm::vec3((x), (y), 0))
            EMIT(0, 0);
            EMIT(1, 0);
            EMIT(1, 1);
            EMIT(0, 1);
#undef EMIT
        }

        tmp_meshes.emplace_back([v = std::move(verts), t = rtex.tex](OpenGL &g) {
            return g.createColoredTexturedQuadBatch(v, t);
        });
    };

    ::foreach_texture(textures, lambda);
    assert(tmp_meshes.size() == numUniqueTextures);
    return tmp_meshes;
}

struct NODISCARD LayerBatchData final
{
    RoomTexVector roomTerrains;
    RoomTexVector roomTrails;
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
    DEFAULT_MOVES(LayerBatchData);
    DELETE_COPIES(LayerBatchData);

    void sort()
    {
        DECL_TIMER(t, "sort");

        /* TODO: Only sort on 2.1 path, since 3.0 can use GL_TEXTURE_2D_ARRAY. */
        roomTerrains.sortByTexture();
        roomTrails.sortByTexture();
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

    NODISCARD LayerMeshesIntermediate buildIntermediate() const
    {
        DECL_TIMER(t2, "LayerBatchData::buildIntermediate");
        LayerMeshesIntermediate meshes;
        meshes.terrain = ::createSortedTexturedMeshes("terrain", roomTerrains);
        meshes.trails = ::createSortedTexturedMeshes("trails", roomTrails);
        meshes.tints = roomTints; // REVISIT: this is a copy instead of a move
        meshes.overlays = ::createSortedTexturedMeshes("overlays", roomOverlays);
        meshes.doors = ::createSortedColoredTexturedMeshes("doors", doors);
        meshes.walls = ::createSortedColoredTexturedMeshes("solidWalls", solidWallLines);
        meshes.dottedWalls = ::createSortedColoredTexturedMeshes("dottedWalls", dottedWallLines);
        meshes.upDownExits = ::createSortedColoredTexturedMeshes("upDownExits", roomUpDownExits);
        meshes.streamIns = ::createSortedColoredTexturedMeshes("streamIns", streamIns);
        meshes.streamOuts = ::createSortedColoredTexturedMeshes("streamOuts", streamOuts);
        meshes.layerBoost = roomLayerBoostQuads; // REVISIT: this is a copy instead of a move
        meshes.isValid = true;
        return meshes;
    }

    NODISCARD LayerMeshes getMeshes(OpenGL &gl) const
    {
        DECL_TIMER(t, "LayerBatchData::getMeshes");
        return buildIntermediate().getLayerMeshes(gl);
    }
};

class NODISCARD LayerBatchBuilder final : public IRoomVisitorCallbacks
{
private:
    LayerBatchData &m_data;
    const mctp::MapCanvasTexturesProxy &m_textures;
    const OptBounds &m_bounds;

public:
    explicit LayerBatchBuilder(LayerBatchData &data,
                               const mctp::MapCanvasTexturesProxy &textures,
                               const OptBounds &bounds)
        : m_data{data}
        , m_textures{textures}
        , m_bounds{bounds}
    {}

    ~LayerBatchBuilder() final;

    DELETE_CTORS_AND_ASSIGN_OPS(LayerBatchBuilder);

private:
    NODISCARD bool virt_acceptRoom(const RoomHandle &room) const final
    {
        return m_bounds.contains(room.getPosition());
    }

    void virt_visitTerrainTexture(const RoomHandle &room, const MMTextureId terrain) final
    {
        if (terrain == INVALID_MM_TEXTURE_ID) {
            return;
        }

        m_data.roomTerrains.emplace_back(room, terrain);

        const auto v0 = room.getPosition().to_vec3();
#define EMIT(x, y) m_data.roomLayerBoostQuads.emplace_back(v0 + glm::vec3((x), (y), 0))
        EMIT(0, 0);
        EMIT(1, 0);
        EMIT(1, 1);
        EMIT(0, 1);
#undef EMIT
    }

    void virt_visitTrailTexture(const RoomHandle &room, const MMTextureId trail) final
    {
        if (trail != INVALID_MM_TEXTURE_ID) {
            m_data.roomTrails.emplace_back(room, trail);
        }
    }

    void virt_visitOverlayTexture(const RoomHandle &room, const MMTextureId overlay) final
    {
        if (overlay != INVALID_MM_TEXTURE_ID) {
            m_data.roomOverlays.emplace_back(room, overlay);
        }
    }

    void virt_visitNamedColorTint(const RoomHandle &room, const RoomTintEnum tint) final
    {
        const auto v0 = room.getPosition().to_vec3();
#define EMIT(x, y) m_data.roomTints[tint].emplace_back(v0 + glm::vec3((x), (y), 0))
        EMIT(0, 0);
        EMIT(1, 0);
        EMIT(1, 1);
        EMIT(0, 1);
#undef EMIT
    }

    void virt_visitWall(const RoomHandle &room,
                        const ExitDirEnum dir,
                        const XNamedColor &color,
                        const WallTypeEnum wallType,
                        const bool isClimb) final
    {
        if (isTransparent(color)) {
            return;
        }

        const std::optional<Color> optColor = getColor(color);
        if (!optColor.has_value()) {
            assert(false);
            return;
        }

        const auto glcolor = optColor.value();

        if (wallType == WallTypeEnum::DOOR) {
            // Note: We could use two door textures (NESW and UD), and then just rotate the
            // texture coordinates, but doing that would require a different code path.
            const MMTextureId tex = m_textures.door[dir];
            m_data.doors.emplace_back(room, tex, glcolor);
        } else {
            if (isNESW(dir)) {
                if (wallType == WallTypeEnum::SOLID) {
                    const MMTextureId tex = m_textures.wall[dir];
                    m_data.solidWallLines.emplace_back(room, tex, glcolor);
                } else {
                    const MMTextureId tex = m_textures.dotted_wall[dir];
                    m_data.dottedWallLines.emplace_back(room, tex, glcolor);
                }
            } else {
                const bool isUp = dir == ExitDirEnum::UP;
                assert(isUp || dir == ExitDirEnum::DOWN);

                const MMTextureId tex = isClimb
                                            ? (isUp ? m_textures.exit_climb_up
                                                    : m_textures.exit_climb_down)
                                            : (isUp ? m_textures.exit_up : m_textures.exit_down);

                m_data.roomUpDownExits.emplace_back(room, tex, glcolor);
            }
        }
    }

    void virt_visitStream(const RoomHandle &room,
                          const ExitDirEnum dir,
                          const StreamTypeEnum type) final
    {
        const Color color = LOOKUP_COLOR(STREAM).getColor();
        switch (type) {
        case StreamTypeEnum::OutFlow:
            m_data.streamOuts.emplace_back(room, m_textures.stream_out[dir], color);
            return;
        case StreamTypeEnum::InFlow:
            m_data.streamIns.emplace_back(room, m_textures.stream_in[dir], color);
            return;
        default:
            break;
        }

        assert(false);
    }
};

LayerBatchBuilder::~LayerBatchBuilder() = default;

NODISCARD static LayerMeshesIntermediate generateLayerMeshes(
    const RoomVector &rooms,
    const mctp::MapCanvasTexturesProxy &textures,
    const OptBounds &bounds,
    const VisitRoomOptions &visitRoomOptions,
    const MumeTimeEnum timeOfDay,
    const std::optional<RoomId> playerRoom,
    const bool playerHasLight)
{
    DECL_TIMER(t, "generateLayerMeshes");

    LayerBatchData data;
    LayerBatchBuilder builder{data, textures, bounds};
    visitRooms(rooms, textures, builder, visitRoomOptions, timeOfDay, playerRoom, playerHasLight);

    data.sort();
    return data.buildIntermediate();
}

struct NODISCARD InternalData final : public IMapBatchesFinisher
{
public:
    std::unordered_map<int, LayerMeshesIntermediate> batchedMeshes;
    BatchedConnections connectionDrawerBuffers;
    std::unordered_map<int, RoomNameBatchIntermediate> roomNameBatches;
    MumeTimeEnum timeOfDay = MumeTimeEnum::UNKNOWN;

private:
    void virt_finish(MapBatches &output, OpenGL &gl, GLFont &font) const final;
};

static void generateAllLayerMeshes(InternalData &internalData,
                                   const FontMetrics &font,
                                   const LayerToRooms &layerToRooms,
                                   const mctp::MapCanvasTexturesProxy &textures,
                                   const VisitRoomOptions &visitRoomOptions,
                                   const MumeTimeEnum timeOfDay,
                                   const std::optional<RoomId> playerRoom,
                                   const bool playerHasLight)

{
    // This feature has been removed, but it's passed to a lot of functions,
    // so it would be annoying to have to add it back if we decide the feature was
    // actually necessary.
    const OptBounds bounds{};

    DECL_TIMER(t, "generateAllLayerMeshes");
    auto &batchedMeshes = internalData.batchedMeshes;
    auto &connectionDrawerBuffers = internalData.connectionDrawerBuffers;
    auto &roomNameBatches = internalData.roomNameBatches;

    for (const auto &layer : layerToRooms) {
        DECL_TIMER(t2, "generateAllLayerMeshes.loop");
        const int thisLayer = layer.first;
        auto &layerMeshes = batchedMeshes[thisLayer];
        ConnectionDrawerBuffers &cdb = connectionDrawerBuffers[thisLayer];
        RoomNameBatch rnb;
        const auto &rooms = layer.second;

        {
            DECL_TIMER(t3, "generateAllLayerMeshes.loop.part2");
            layerMeshes = ::generateLayerMeshes(rooms,
                                                textures,
                                                bounds,
                                                visitRoomOptions,
                                                timeOfDay,
                                                playerRoom,
                                                playerHasLight);
        }

        {
            DECL_TIMER(t4, "generateAllLayerMeshes.loop.part3");

            // TODO: move everything in the same layer to the same internal struct?
            cdb.clear();
            rnb.clear();

            ConnectionDrawer cd{cdb, rnb, thisLayer, bounds};
            {
                DECL_TIMER(t7, "generateAllLayerMeshes.loop.part3b");
                // pass 2: add to buffers
                for (const auto &room : rooms) {
                    cd.drawRoomConnectionsAndDoors(room);
                }
            }
        }

        {
            DECL_TIMER(t8, "generateAllLayerMeshes.loop.part4");
            roomNameBatches[thisLayer] = rnb.getIntermediate(font);
        }
    }
}

LayerMeshes LayerMeshesIntermediate::getLayerMeshes(OpenGL &gl) const
{
    if (!isValid) {
        return {};
    }

    struct NODISCARD Resolver final
    {
        OpenGL &m_gl;
        NODISCARD UniqueMeshVector resolve(const FnVec &v)
        {
            std::vector<UniqueMesh> result;
            result.reserve(v.size());
            for (const auto &f : v) {
                result.emplace_back(f(m_gl));
            }
            return UniqueMeshVector{std::move(result)};
        }
        NODISCARD UniqueMesh resolve(const PlainQuadBatch &batch)
        {
            return m_gl.createPlainQuadBatch(batch);
        }
        NODISCARD RoomTintArray<UniqueMesh> resolve(const RoomTintArray<PlainQuadBatch> &arr)
        {
            RoomTintArray<UniqueMesh> result;
            for (const RoomTintEnum tint : ALL_ROOM_TINTS) {
                const PlainQuadBatch &b = arr[tint];
                result[tint] = m_gl.createPlainQuadBatch(b);
            }
            return result;
        }
    };

    DECL_TIMER(t, "LayerMeshesIntermediate::getLayerMeshes");
    Resolver r{gl};
    LayerMeshes lm;
    lm.terrain = r.resolve(terrain);
    lm.trails = r.resolve(trails);
    lm.tints = r.resolve(tints);
    lm.overlays = r.resolve(overlays);
    lm.doors = r.resolve(doors);
    lm.walls = r.resolve(walls);
    lm.dottedWalls = r.resolve(dottedWalls);
    lm.upDownExits = r.resolve(upDownExits);
    lm.streamIns = r.resolve(streamIns);
    lm.streamOuts = r.resolve(streamOuts);
    lm.layerBoost = r.resolve(layerBoost);
    lm.isValid = true;

    return lm;
}

void LayerMeshes::render(const int thisLayer,
                         const int focusedLayer,
                         const glm::vec3 &playerPos,
                         const bool isNight)
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

    GLRenderState less = GLRenderState().withDepthFunction(DepthFunctionEnum::LESS);
    GLRenderState equal = GLRenderState().withDepthFunction(DepthFunctionEnum::EQUAL);
    GLRenderState lequal = GLRenderState().withDepthFunction(DepthFunctionEnum::LEQUAL);

    // Set player position and layer for radial transparency
    const bool enableRadialTransparency = getConfig().canvas.enableRadialTransparency;
    less.uniforms.playerPos = playerPos;
    less.uniforms.currentLayer = focusedLayer;
    less.uniforms.enableRadialTransparency = enableRadialTransparency;
    less.uniforms.texturesDisabled = disableTextures;
    less.uniforms.isNight = isNight;

    equal.uniforms.playerPos = playerPos;
    equal.uniforms.currentLayer = focusedLayer;
    equal.uniforms.enableRadialTransparency = enableRadialTransparency;
    equal.uniforms.texturesDisabled = disableTextures;
    equal.uniforms.isNight = isNight;

    lequal.uniforms.playerPos = playerPos;
    lequal.uniforms.currentLayer = focusedLayer;
    lequal.uniforms.enableRadialTransparency = enableRadialTransparency;
    lequal.uniforms.texturesDisabled = disableTextures;
    lequal.uniforms.isNight = isNight;

    const GLRenderState less_blended = less.withBlend(BlendModeEnum::TRANSPARENCY);
    const GLRenderState lequal_blended = lequal.withBlend(BlendModeEnum::TRANSPARENCY);
    const GLRenderState equal_blended = equal.withBlend(BlendModeEnum::TRANSPARENCY);
    const GLRenderState equal_multiplied = equal.withBlend(BlendModeEnum::MODULATE);

    const float layerTransparency = getConfig().canvas.layerTransparency;
    const auto color = [&thisLayer, &focusedLayer, &disableTextures, layerTransparency]() {
        if (thisLayer == focusedLayer) {
            // Focused layer: full opacity
            return Colors::white.withAlpha(0.90f);
        } else if (thisLayer > focusedLayer) {
            // Upper layers: render up to 10 layers above with progressive fading
            const int layersAbove = thisLayer - focusedLayer;
            if (layersAbove > 10) {
                // More than 10 layers above: don't render
                return Colors::gray70.withAlpha(0.0f);
            }
            // User-controlled transparency: at slider=1, layer 1=1.0, layer 2=0.9, ..., layer 10=0.1
            // Base alpha decreases by 0.1 per layer: 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1
            float baseAlpha = glm::clamp(1.1f - (layersAbove * 0.1f), 0.1f, 1.0f);
            float opacity = baseAlpha * layerTransparency;
            if (disableTextures) {
                // Non-textured: use 70% of the opacity for better distinction
                return Colors::gray70.withAlpha(opacity * 0.7f);
            } else {
                return Colors::gray70.withAlpha(opacity);
            }
        } else {
            // Layers below focused: calculate decreasing opacity
            const int layersBelow = focusedLayer - thisLayer;
            // User-controlled transparency: at slider=1, layer 1=1.0, layer 2=0.9, ..., layer 10=0.1
            // Base alpha decreases by 0.1 per layer: 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1
            float baseAlpha = glm::clamp(1.1f - (layersBelow * 0.1f), 0.1f, 1.0f);
            float opacity = baseAlpha * layerTransparency;
            return Colors::white.withAlpha(opacity);
        }
    }();

    {
        /* REVISIT: For the modern case, we could render each layer separately,
         * and then only blend the layers that actually overlap. Doing that would
         * give higher contrast for the base textures.
         */
        if (disableTextures) {
            // Use the same opacity calculation as textured layers
            layerBoost.render(less_blended.withColor(color));
        } else {
            terrain.render(less_blended.withColor(color));
        }
    }

    // REVISIT: move trails to their own batch also colored by the tint?
    // Only render tints on the focused layer
    // Tints use MODULATE blend which multiplies colors - this works well on opaque layers
    // but creates washed-out appearance on transparent layers
    if (thisLayer == focusedLayer) {
        // Only render tints on the focused layer to avoid appearance changes with slider adjustments
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
                // Use original modulate blend at full strength for focused layer
                tints[tint].render(equal_multiplied.withColor(optColor.value()));
            } else {
                assert(false);
            }
        }
    }

    if (!disableTextures) {
        // streams go under everything else, including trails
        streamIns.render(lequal_blended.withColor(color));
        streamOuts.render(lequal_blended.withColor(color));

        trails.render(equal_blended.withColor(color));
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

    // Don't apply any additional darkening overlay since we're using opacity-based fading
    // The original code used a black overlay which made layers too dark when combined with opacity fading
}

void InternalData::virt_finish(MapBatches &output, OpenGL &gl, GLFont &font) const
{
    DECL_TIMER(t, "InternalData::virt_finish");

    // Set night status for shader darkening
    output.isNight = (timeOfDay == MumeTimeEnum::NIGHT || timeOfDay == MumeTimeEnum::DUSK);

    {
        DECL_TIMER(t2, "InternalData::virt_finish batchedMeshes");
        for (const auto &kv : batchedMeshes) {
            const LayerMeshesIntermediate &data = kv.second;
            output.batchedMeshes[kv.first] = data.getLayerMeshes(gl);
        }
    }
    {
        DECL_TIMER(t2, "InternalData::virt_finish connectionMeshes");
        for (const auto &kv : connectionDrawerBuffers) {
            const ConnectionDrawerBuffers &data = kv.second;
            output.connectionMeshes[kv.first] = data.getMeshes(gl);
        }
    }

    {
        DECL_TIMER(t2, "InternalData::virt_finish roomNameBatches");
        for (const auto &kv : roomNameBatches) {
            const RoomNameBatchIntermediate &rnb = kv.second;
            output.roomNameBatches[kv.first] = rnb.getMesh(font);
        }
    }
}

// NOTE: All of the lamda captures are copied, including the texture data!
FutureSharedMapBatchFinisher generateMapDataFinisher(const mctp::MapCanvasTexturesProxy &textures,
                                                     const std::shared_ptr<const FontMetrics> &font,
                                                     const Map &map,
                                                     const MumeTimeEnum timeOfDay,
                                                     const std::optional<RoomId> playerRoom,
                                                     const bool playerHasLight)
{
    const auto visitRoomOptions = getVisitRoomOptions();

    return std::async(std::launch::async,
                      [textures, font, map, visitRoomOptions, timeOfDay, playerRoom, playerHasLight]()
                          -> SharedMapBatchFinisher {
                          ThreadLocalNamedColorRaii tlRaii{visitRoomOptions.canvasColors,
                                                           visitRoomOptions.colorSettings};
                          DECL_TIMER(t, "[ASYNC] generateAllLayerMeshes");

                          ProgressCounter dummyPc;
                          map.checkConsistency(dummyPc);

                          const LayerToRooms layerToRooms = [map]() -> LayerToRooms {
                              DECL_TIMER(t2, "[ASYNC] generateBatches.layerToRooms");
                              LayerToRooms ltr;
                              map.getRooms().for_each([&map, &ltr](const RoomId id) {
                                  const auto &r = map.getRoomHandle(id);
                                  const auto z = r.getPosition().z;
                                  auto &layer = ltr[z];
                                  layer.emplace_back(r);
                              });
                              return ltr;
                          }();

                          auto result = std::make_shared<InternalData>();
                          auto &data = deref(result);
                          data.timeOfDay = timeOfDay;
                          generateAllLayerMeshes(data,
                                                 deref(font),
                                                 layerToRooms,
                                                 textures,
                                                 visitRoomOptions,
                                                 timeOfDay,
                                                 playerRoom,
                                                 playerHasLight);
                          return SharedMapBatchFinisher{result};
                      });
}

void finish(const IMapBatchesFinisher &finisher,
            std::optional<MapBatches> &opt_batches,
            OpenGL &gl,
            GLFont &font)
{
    MapBatches &batches = opt_batches.emplace();

    // Note: This will call InternalData::finish;
    // if necessary for claritiy, we could replace this with Pimpl to make it a direct call,
    // but that won't change the cost of the virtual call.
    finisher.finish(batches, gl, font);
}
