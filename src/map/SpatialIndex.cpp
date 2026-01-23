// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "SpatialIndex.h"

#include "../global/AnsiOstream.h"
#include "../global/progresscounter.h"

#include <algorithm>
#include <cassert>

namespace spatial {

// ============================================================================
// QuadtreeNode implementation
// ============================================================================

QuadtreeNode::QuadtreeNode(const int minX, const int minY, const int maxX, const int maxY)
    : m_minX{minX}
    , m_minY{minY}
    , m_maxX{maxX}
    , m_maxY{maxY}
{}

QuadtreeNode::~QuadtreeNode() = default;

QuadtreeNode::QuadtreeNode(const QuadtreeNode &other)
    : m_minX{other.m_minX}
    , m_minY{other.m_minY}
    , m_maxX{other.m_maxX}
    , m_maxY{other.m_maxY}
    , m_rooms{other.m_rooms}
    , m_isLeaf{other.m_isLeaf}
{
    for (size_t i = 0; i < NUM_QUADRANTS; ++i) {
        if (other.m_children[i]) {
            m_children[i] = std::make_unique<QuadtreeNode>(*other.m_children[i]);
        }
    }
}

bool QuadtreeNode::contains(const int x, const int y) const
{
    return x >= m_minX && x < m_maxX && y >= m_minY && y < m_maxY;
}

QuadtreeNode::Quadrant QuadtreeNode::getQuadrant(const int x, const int y) const
{
    const int centerX = getCenterX();
    const int centerY = getCenterY();

    if (x < centerX) {
        return (y >= centerY) ? Quadrant::NorthWest : Quadrant::SouthWest;
    } else {
        return (y >= centerY) ? Quadrant::NorthEast : Quadrant::SouthEast;
    }
}

bool QuadtreeNode::shouldSubdivide() const
{
    if (!m_isLeaf) {
        return false;
    }

    // Don't subdivide if too small
    if (getWidth() <= QuadtreeConfig::MIN_SQUARE_SIZE
        || getHeight() <= QuadtreeConfig::MIN_SQUARE_SIZE) {
        return false;
    }

    // Count total rooms across all coordinates
    size_t totalRooms = 0;
    for (const auto &[coord, rooms] : m_rooms) {
        totalRooms += rooms.size();
    }

    return totalRooms > QuadtreeConfig::MAX_LEAF_ROOMS;
}

void QuadtreeNode::subdivide()
{
    if (!m_isLeaf) {
        return;
    }

    const int centerX = getCenterX();
    const int centerY = getCenterY();

    // Create child nodes
    m_children[static_cast<size_t>(Quadrant::NorthWest)] =
        std::make_unique<QuadtreeNode>(m_minX, centerY, centerX, m_maxY);
    m_children[static_cast<size_t>(Quadrant::NorthEast)] =
        std::make_unique<QuadtreeNode>(centerX, centerY, m_maxX, m_maxY);
    m_children[static_cast<size_t>(Quadrant::SouthWest)] =
        std::make_unique<QuadtreeNode>(m_minX, m_minY, centerX, centerY);
    m_children[static_cast<size_t>(Quadrant::SouthEast)] =
        std::make_unique<QuadtreeNode>(centerX, m_minY, m_maxX, centerY);

    m_isLeaf = false;

    // Move existing rooms to children
    for (auto &[coord, rooms] : m_rooms) {
        for (const RoomId id : rooms) {
            insertIntoChild(id, coord.x, coord.y);
        }
    }
    m_rooms.clear();
}

void QuadtreeNode::insertIntoChild(const RoomId id, const int x, const int y)
{
    const auto quadrant = getQuadrant(x, y);
    auto &child = m_children[static_cast<size_t>(quadrant)];

    // Create child on-demand if it doesn't exist
    if (!child) {
        const int centerX = getCenterX();
        const int centerY = getCenterY();

        switch (quadrant) {
        case Quadrant::NorthWest:
            child = std::make_unique<QuadtreeNode>(m_minX, centerY, centerX, m_maxY);
            break;
        case Quadrant::NorthEast:
            child = std::make_unique<QuadtreeNode>(centerX, centerY, m_maxX, m_maxY);
            break;
        case Quadrant::SouthWest:
            child = std::make_unique<QuadtreeNode>(m_minX, m_minY, centerX, centerY);
            break;
        case Quadrant::SouthEast:
            child = std::make_unique<QuadtreeNode>(centerX, m_minY, m_maxX, centerY);
            break;
        }
    }

    child->insert(id, x, y);
}

void QuadtreeNode::insert(const RoomId id, const int x, const int y)
{
    if (m_isLeaf) {
        const Coordinate2i coord{x, y};
        m_rooms[coord].insert(id);

        if (shouldSubdivide()) {
            subdivide();
        }
    } else {
        insertIntoChild(id, x, y);
    }
}

void QuadtreeNode::remove(const RoomId id, const int x, const int y)
{
    if (m_isLeaf) {
        const Coordinate2i coord{x, y};
        if (auto it = m_rooms.find(coord); it != m_rooms.end()) {
            it->second.erase(id);
            if (it->second.empty()) {
                m_rooms.erase(it);
            }
        }
    } else {
        const auto quadrant = getQuadrant(x, y);
        if (auto &child = m_children[static_cast<size_t>(quadrant)]) {
            child->remove(id, x, y);
        }
    }
}

TinyRoomIdSet QuadtreeNode::findAt(const int x, const int y) const
{
    if (m_isLeaf) {
        const Coordinate2i coord{x, y};
        if (auto it = m_rooms.find(coord); it != m_rooms.end()) {
            return it->second;
        }
        return {};
    }

    const auto quadrant = getQuadrant(x, y);
    if (const auto &child = m_children[static_cast<size_t>(quadrant)]) {
        return child->findAt(x, y);
    }
    return {};
}

TinyRoomIdSet QuadtreeNode::findInBounds(const int minX,
                                         const int minY,
                                         const int maxX,
                                         const int maxY) const
{
    // Check if this node intersects the query bounds
    if (m_maxX <= minX || m_minX >= maxX || m_maxY <= minY || m_minY >= maxY) {
        return {};
    }

    TinyRoomIdSet result;

    if (m_isLeaf) {
        for (const auto &[coord, rooms] : m_rooms) {
            if (coord.x >= minX && coord.x < maxX && coord.y >= minY && coord.y < maxY) {
                result.insertAll(rooms);
            }
        }
    } else {
        for (const auto &child : m_children) {
            if (child) {
                result.insertAll(child->findInBounds(minX, minY, maxX, maxY));
            }
        }
    }

    return result;
}

size_t QuadtreeNode::countRooms() const
{
    if (m_isLeaf) {
        size_t count = 0;
        for (const auto &[coord, rooms] : m_rooms) {
            count += rooms.size();
        }
        return count;
    }

    size_t count = 0;
    for (const auto &child : m_children) {
        if (child) {
            count += child->countRooms();
        }
    }
    return count;
}

// ============================================================================
// Plane implementation
// ============================================================================

Plane::Plane(const int z)
    : m_z{z}
{}

Plane::~Plane() = default;

Plane::Plane(const Plane &other)
    : m_z{other.m_z}
{
    if (other.m_root) {
        m_root = std::make_unique<QuadtreeNode>(*other.m_root);
    }
}

void Plane::ensureContains(const int x, const int y)
{
    if (!m_root) {
        // Create initial root centered around the first point
        const int halfSize = QuadtreeConfig::INITIAL_HALF_SIZE;
        m_root = std::make_unique<QuadtreeNode>(x - halfSize, y - halfSize, x + halfSize, y + halfSize);
        return;
    }

    // Expand root if point is outside bounds
    // We must always double in both dimensions so old root fits exactly into one quadrant
    while (!m_root->contains(x, y)) {
        const int oldMinX = m_root->getMinX();
        const int oldMinY = m_root->getMinY();
        const int oldMaxX = m_root->getMaxX();
        const int oldMaxY = m_root->getMaxY();
        const int width = oldMaxX - oldMinX;
        const int height = oldMaxY - oldMinY;

        // Determine expansion direction based on where the point is
        // The old root will occupy one quadrant of the new root
        const bool expandWest = (x < oldMinX);
        const bool expandEast = (x >= oldMaxX);
        const bool expandSouth = (y < oldMinY);
        const bool expandNorth = (y >= oldMaxY);

        int newMinX, newMinY, newMaxX, newMaxY;
        QuadtreeNode::Quadrant oldRootQuadrant;

        // Always double the size in both dimensions
        // Choose direction based on where the point is, defaulting to NE expansion
        if (expandWest || (!expandEast && !expandWest)) {
            // Expand west (or default): old root goes to east side
            newMinX = oldMinX - width;
            newMaxX = oldMaxX;
        } else {
            // Expand east: old root goes to west side
            newMinX = oldMinX;
            newMaxX = oldMaxX + width;
        }

        if (expandSouth || (!expandNorth && !expandSouth)) {
            // Expand south (or default): old root goes to north side
            newMinY = oldMinY - height;
            newMaxY = oldMaxY;
        } else {
            // Expand north: old root goes to south side
            newMinY = oldMinY;
            newMaxY = oldMaxY + height;
        }

        // Determine which quadrant the old root occupies in the new root
        // Old root is at [oldMinX, oldMaxX) x [oldMinY, oldMaxY)
        // New center is at (newMinX + width, newMinY + height)
        const int newCenterX = newMinX + width;
        const int newCenterY = newMinY + height;

        // Old root's center determines its quadrant
        const int oldCenterX = oldMinX + width / 2;
        const int oldCenterY = oldMinY + height / 2;

        if (oldCenterX < newCenterX) {
            oldRootQuadrant = (oldCenterY >= newCenterY) ? QuadtreeNode::Quadrant::NorthWest
                                                         : QuadtreeNode::Quadrant::SouthWest;
        } else {
            oldRootQuadrant = (oldCenterY >= newCenterY) ? QuadtreeNode::Quadrant::NorthEast
                                                         : QuadtreeNode::Quadrant::SouthEast;
        }

        // Create new root with doubled size
        auto newRoot = std::make_unique<QuadtreeNode>(newMinX, newMinY, newMaxX, newMaxY);

        // The old root becomes one of the children - it should fit exactly
        newRoot->m_children[static_cast<size_t>(oldRootQuadrant)] = std::move(m_root);
        newRoot->m_isLeaf = false;

        m_root = std::move(newRoot);
    }
}

void Plane::insert(const RoomId id, const int x, const int y)
{
    ensureContains(x, y);
    m_root->insert(id, x, y);
}

void Plane::remove(const RoomId id, const int x, const int y)
{
    if (m_root) {
        m_root->remove(id, x, y);
    }
}

TinyRoomIdSet Plane::findAt(const int x, const int y) const
{
    if (m_root && m_root->contains(x, y)) {
        return m_root->findAt(x, y);
    }
    return {};
}

TinyRoomIdSet Plane::findInBounds(const int minX, const int minY, const int maxX, const int maxY) const
{
    if (m_root) {
        return m_root->findInBounds(minX, minY, maxX, maxY);
    }
    return {};
}

size_t Plane::countRooms() const
{
    return m_root ? m_root->countRooms() : 0;
}

} // namespace spatial

// ============================================================================
// SpatialIndex implementation
// ============================================================================

SpatialIndex::SpatialIndex() = default;
SpatialIndex::~SpatialIndex() = default;

SpatialIndex::SpatialIndex(const SpatialIndex &other)
    : m_bounds{other.m_bounds}
    , m_needsBoundsUpdate{other.m_needsBoundsUpdate}
{
    for (const auto &[z, plane] : other.m_planes) {
        m_planes.emplace(z, std::make_unique<spatial::Plane>(*plane));
    }
}

SpatialIndex &SpatialIndex::operator=(const SpatialIndex &other)
{
    if (this != &other) {
        m_planes.clear();
        for (const auto &[z, plane] : other.m_planes) {
            m_planes.emplace(z, std::make_unique<spatial::Plane>(*plane));
        }
        m_bounds = other.m_bounds;
        m_needsBoundsUpdate = other.m_needsBoundsUpdate;
    }
    return *this;
}

spatial::Plane &SpatialIndex::getOrCreatePlane(const int z)
{
    auto it = m_planes.find(z);
    if (it == m_planes.end()) {
        auto [newIt, inserted] = m_planes.emplace(z, std::make_unique<spatial::Plane>(z));
        return *newIt->second;
    }
    return *it->second;
}

const spatial::Plane *SpatialIndex::findPlane(const int z) const
{
    auto it = m_planes.find(z);
    return (it != m_planes.end()) ? it->second.get() : nullptr;
}

void SpatialIndex::invalidateBounds()
{
    m_needsBoundsUpdate = true;
}

void SpatialIndex::insert(const RoomId id, const Coordinate &coord)
{
    auto &plane = getOrCreatePlane(coord.z);
    plane.insert(id, coord.x, coord.y);
    invalidateBounds();
}

void SpatialIndex::remove(const RoomId id, const Coordinate &coord)
{
    if (auto *plane = findPlane(coord.z)) {
        // Note: We use const_cast here because we need to modify the plane
        // but findPlane returns const for const correctness in queries
        const_cast<spatial::Plane *>(plane)->remove(id, coord.x, coord.y);
        invalidateBounds();
    }
}

void SpatialIndex::move(const RoomId id, const Coordinate &from, const Coordinate &to)
{
    if (from == to) {
        return;
    }
    remove(id, from);
    insert(id, to);
}

TinyRoomIdSet SpatialIndex::findAt(const Coordinate &coord) const
{
    if (const auto *plane = findPlane(coord.z)) {
        return plane->findAt(coord.x, coord.y);
    }
    return {};
}

std::optional<RoomId> SpatialIndex::findFirst(const Coordinate &coord) const
{
    const auto rooms = findAt(coord);
    if (!rooms.empty()) {
        return rooms.first();
    }
    return std::nullopt;
}

bool SpatialIndex::hasRoomAt(const Coordinate &coord) const
{
    return !findAt(coord).empty();
}

TinyRoomIdSet SpatialIndex::findInBounds(const Bounds &bounds) const
{
    TinyRoomIdSet result;

    for (const auto &[z, plane] : m_planes) {
        if (z >= bounds.min.z && z <= bounds.max.z) {
            result.insertAll(plane->findInBounds(bounds.min.x, bounds.min.y, bounds.max.x + 1, bounds.max.y + 1));
        }
    }

    return result;
}

TinyRoomIdSet SpatialIndex::findInRadius(const Coordinate &center, const int radius) const
{
    // For now, use bounding box approximation
    // Future: could implement proper circular distance check
    const Bounds bounds{
        Coordinate{center.x - radius, center.y - radius, center.z - radius},
        Coordinate{center.x + radius, center.y + radius, center.z + radius}};
    return findInBounds(bounds);
}

size_t SpatialIndex::size() const
{
    size_t total = 0;
    for (const auto &[z, plane] : m_planes) {
        total += plane->countRooms();
    }
    return total;
}

bool SpatialIndex::empty() const
{
    for (const auto &[z, plane] : m_planes) {
        if (plane->countRooms() > 0) {
            return false;
        }
    }
    return true;
}

std::optional<Bounds> SpatialIndex::getBounds() const
{
    if (!m_needsBoundsUpdate && m_bounds.has_value()) {
        return m_bounds;
    }

    // Compute bounds by iterating all rooms
    std::optional<Bounds> bounds;
    forEach([&bounds](const RoomId /*id*/, const Coordinate &coord) {
        if (!bounds.has_value()) {
            bounds.emplace(coord, coord);
        } else {
            bounds->insert(coord);
        }
    });

    m_bounds = bounds;
    m_needsBoundsUpdate = false;
    return m_bounds;
}

void SpatialIndex::updateBounds(ProgressCounter &pc)
{
    m_bounds.reset();
    m_needsBoundsUpdate = false;

    if (empty()) {
        return;
    }

    const size_t total = size();
    pc.increaseTotalStepsBy(total);

    forEach([this, &pc](const RoomId /*id*/, const Coordinate &coord) {
        if (!m_bounds.has_value()) {
            m_bounds.emplace(coord, coord);
        } else {
            m_bounds->insert(coord);
        }
        pc.step();
    });
}

void SpatialIndex::printStats(ProgressCounter & /*pc*/, AnsiOstream &os) const
{
    const auto bounds = getBounds();
    if (bounds.has_value()) {
        const Coordinate &max = bounds->max;
        const Coordinate &min = bounds->min;

        static constexpr auto green = getRawAnsi(AnsiColor16Enum::green);

        auto show = [&os](std::string_view prefix, int lo, int hi) {
            os << prefix << ColoredValue(green, hi - lo + 1) << " (" << ColoredValue(green, lo)
               << " to " << ColoredValue(green, hi) << ").\n";
        };

        os << "\n";
        show("Width  (West  to East):   ", min.x, max.x);
        show("Height (South to North):  ", min.y, max.y);
        show("Layers (Down  to Up):     ", min.z, max.z);

        os << "\nSpatial Index Statistics:\n";
        os << "  Total rooms: " << ColoredValue(green, size()) << "\n";
        os << "  Z-planes: " << ColoredValue(green, m_planes.size()) << "\n";
    }
}

bool SpatialIndex::operator==(const SpatialIndex &other) const
{
    if (size() != other.size()) {
        return false;
    }

    // Compare all rooms
    bool equal = true;
    forEach([&other, &equal](const RoomId id, const Coordinate &coord) {
        if (equal) {
            const auto otherRooms = other.findAt(coord);
            if (!otherRooms.contains(id)) {
                equal = false;
            }
        }
    });

    return equal;
}
