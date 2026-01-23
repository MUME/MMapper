#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"
#include "TinyRoomIdSet.h"
#include "coordinate.h"
#include "roomid.h"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class AnsiOstream;
class ProgressCounter;

namespace spatial {

/// Configuration constants for the quadtree
struct NODISCARD QuadtreeConfig final
{
    /// Maximum rooms in a leaf node before subdivision
    static constexpr int MAX_LEAF_ROOMS = 32;
    /// Minimum square size (won't subdivide smaller than this)
    static constexpr int MIN_SQUARE_SIZE = 4;
    /// Initial square half-size when creating a new plane
    static constexpr int INITIAL_HALF_SIZE = 64;
};

/// A node in the quadtree, representing a square region of the XY plane
class NODISCARD QuadtreeNode final
{
    friend class Plane; // Plane needs access to expand the tree

public:
    enum class NODISCARD Quadrant : uint8_t
    {
        NorthWest = 0, // x < center, y >= center
        NorthEast = 1, // x >= center, y >= center
        SouthWest = 2, // x < center, y < center
        SouthEast = 3  // x >= center, y < center
    };
    static constexpr size_t NUM_QUADRANTS = 4;

private:
    /// Bounds of this node
    int m_minX = 0;
    int m_minY = 0;
    int m_maxX = 0;
    int m_maxY = 0;

    /// Child nodes (nullptr if leaf)
    std::array<std::unique_ptr<QuadtreeNode>, NUM_QUADRANTS> m_children;

    /// Rooms stored in this leaf node (empty if internal node)
    /// Maps coordinate (x,y only, z is handled by Plane) to room set
    std::unordered_map<Coordinate2i, TinyRoomIdSet> m_rooms;

    /// Whether this is a leaf node
    bool m_isLeaf = true;

public:
    QuadtreeNode(int minX, int minY, int maxX, int maxY);
    ~QuadtreeNode();

    QuadtreeNode(const QuadtreeNode &other);
    QuadtreeNode &operator=(const QuadtreeNode &) = delete;
    QuadtreeNode(QuadtreeNode &&) = default;
    QuadtreeNode &operator=(QuadtreeNode &&) = default;

public:
    NODISCARD int getMinX() const { return m_minX; }
    NODISCARD int getMinY() const { return m_minY; }
    NODISCARD int getMaxX() const { return m_maxX; }
    NODISCARD int getMaxY() const { return m_maxY; }
    NODISCARD int getCenterX() const { return m_minX + (m_maxX - m_minX) / 2; }
    NODISCARD int getCenterY() const { return m_minY + (m_maxY - m_minY) / 2; }
    NODISCARD int getWidth() const { return m_maxX - m_minX; }
    NODISCARD int getHeight() const { return m_maxY - m_minY; }
    NODISCARD bool isLeaf() const { return m_isLeaf; }

public:
    NODISCARD bool contains(int x, int y) const;
    NODISCARD Quadrant getQuadrant(int x, int y) const;

public:
    void insert(RoomId id, int x, int y);
    void remove(RoomId id, int x, int y);
    NODISCARD TinyRoomIdSet findAt(int x, int y) const;
    NODISCARD TinyRoomIdSet findInBounds(int minX, int minY, int maxX, int maxY) const;

    template<typename Callback>
    void forEach(Callback &&callback) const;

    NODISCARD size_t countRooms() const;

private:
    void subdivide();
    NODISCARD bool shouldSubdivide() const;
    void insertIntoChild(RoomId id, int x, int y);
};

/// A plane represents all rooms at a single Z level
class NODISCARD Plane final
{
private:
    int m_z = 0;
    std::unique_ptr<QuadtreeNode> m_root;

public:
    explicit Plane(int z);
    ~Plane();

    Plane(const Plane &other);
    Plane &operator=(const Plane &) = delete;
    Plane(Plane &&) = default;
    Plane &operator=(Plane &&) = default;

public:
    NODISCARD int getZ() const { return m_z; }

public:
    void insert(RoomId id, int x, int y);
    void remove(RoomId id, int x, int y);
    NODISCARD TinyRoomIdSet findAt(int x, int y) const;
    NODISCARD TinyRoomIdSet findInBounds(int minX, int minY, int maxX, int maxY) const;

    template<typename Callback>
    void forEach(Callback &&callback) const;

    NODISCARD size_t countRooms() const;

private:
    void ensureContains(int x, int y);
};

} // namespace spatial

/// Main spatial index class that replaces SpatialDb
/// Supports multiple rooms per coordinate via quadtree organization
class NODISCARD SpatialIndex final
{
private:
    /// Maps Z coordinate to plane
    std::unordered_map<int, std::unique_ptr<spatial::Plane>> m_planes;

    /// Cached bounds (lazily updated)
    mutable std::optional<Bounds> m_bounds;
    mutable bool m_needsBoundsUpdate = true;

public:
    SpatialIndex();
    ~SpatialIndex();

    SpatialIndex(const SpatialIndex &other);
    SpatialIndex &operator=(const SpatialIndex &other);
    SpatialIndex(SpatialIndex &&) = default;
    SpatialIndex &operator=(SpatialIndex &&) = default;

public:
    /// Insert a room at the given coordinate
    void insert(RoomId id, const Coordinate &coord);

    /// Remove a room from the given coordinate
    void remove(RoomId id, const Coordinate &coord);

    /// Move a room from one coordinate to another
    void move(RoomId id, const Coordinate &from, const Coordinate &to);

public:
    /// Find all rooms at exact coordinate
    NODISCARD TinyRoomIdSet findAt(const Coordinate &coord) const;

    /// Find first room at coordinate (for backward compatibility)
    NODISCARD std::optional<RoomId> findFirst(const Coordinate &coord) const;

    /// Check if any room exists at coordinate
    NODISCARD bool hasRoomAt(const Coordinate &coord) const;

public:
    /// Find all rooms within a bounding box
    NODISCARD TinyRoomIdSet findInBounds(const Bounds &bounds) const;

    /// Find all rooms within radius (for future float coordinate support)
    NODISCARD TinyRoomIdSet findInRadius(const Coordinate &center, int radius) const;

public:
    /// Iterate over all rooms
    template<typename Callback>
    void forEach(Callback &&callback) const;

    /// Get total number of coordinate entries
    NODISCARD size_t size() const;

    /// Check if index is empty
    NODISCARD bool empty() const;

public:
    /// Get bounds of all rooms (computed lazily)
    NODISCARD std::optional<Bounds> getBounds() const;

    /// Check if bounds need recalculation
    NODISCARD bool needsBoundsUpdate() const { return m_needsBoundsUpdate; }

    /// Force bounds recalculation
    void updateBounds(ProgressCounter &pc);

public:
    /// Print statistics
    void printStats(ProgressCounter &pc, AnsiOstream &os) const;

public:
    NODISCARD bool operator==(const SpatialIndex &other) const;
    NODISCARD bool operator!=(const SpatialIndex &other) const { return !operator==(other); }

private:
    spatial::Plane &getOrCreatePlane(int z);
    const spatial::Plane *findPlane(int z) const;
    void invalidateBounds();
};

// Template implementations

template<typename Callback>
void spatial::QuadtreeNode::forEach(Callback &&callback) const
{
    if (m_isLeaf) {
        for (const auto &[coord, rooms] : m_rooms) {
            for (const RoomId id : rooms) {
                callback(id, coord);
            }
        }
    } else {
        for (const auto &child : m_children) {
            if (child) {
                child->forEach(std::forward<Callback>(callback));
            }
        }
    }
}

template<typename Callback>
void spatial::Plane::forEach(Callback &&callback) const
{
    if (m_root) {
        m_root->forEach([this, &callback](RoomId id, const Coordinate2i &xy) {
            callback(id, Coordinate{xy, m_z});
        });
    }
}

template<typename Callback>
void SpatialIndex::forEach(Callback &&callback) const
{
    for (const auto &[z, plane] : m_planes) {
        plane->forEach(std::forward<Callback>(callback));
    }
}
