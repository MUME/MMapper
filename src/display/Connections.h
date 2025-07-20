#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Array.h"
#include "../global/RuleOf5.h"
#include "../global/utils.h"
#include "../map/ExitDirection.h"
#include "../map/RoomHandle.h"
#include "../map/coordinate.h"
#include "../map/roomid.h"
#include "../opengl/Font.h"
#include "../opengl/OpenGLTypes.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include <QString>

class GLFont;
class OpenGL;
struct FontMetrics;

struct NODISCARD RoomNameBatchIntermediate final
{
    std::vector<FontVert3d> verts;

    NODISCARD UniqueMesh getMesh(GLFont &gl) const;
    NODISCARD bool empty() const { return verts.empty(); }
    void clear() { verts.clear(); }

    template<typename T>
    static void append(std::vector<T> &v, const std::vector<T> &other)
    {
        v.insert(v.end(), other.begin(), other.end());
    }
    void append(const std::vector<FontVert3d> &other) { append(verts, other); }
};

struct NODISCARD RoomNameBatch final
{
private:
    std::vector<GLText> m_names;

public:
    RoomNameBatch() = default;
    DEFAULT_MOVES_DELETE_COPIES(RoomNameBatch);
    ~RoomNameBatch() = default;

public:
    void emplace_back(GLText &&glt) { m_names.emplace_back(std::move(glt)); }

public:
    void reserve(const size_t elements) { m_names.reserve(elements); }
    NODISCARD size_t size() const { return m_names.size(); }
    void clear() { m_names.clear(); }
    NODISCARD bool empty() const { return m_names.empty(); }

public:
    NODISCARD RoomNameBatchIntermediate getIntermediate(const FontMetrics &font) const;
};

using BatchedRoomNames = std::unordered_map<int, UniqueMesh>;

struct NODISCARD ConnectionDrawerColorBuffer final
{
    std::vector<ColorVert> triVerts;
    std::vector<ColorVert> quadVerts;

    ConnectionDrawerColorBuffer() = default;
    DEFAULT_MOVES_DELETE_COPIES(ConnectionDrawerColorBuffer);
    ~ConnectionDrawerColorBuffer() = default;

    void clear()
    {
        triVerts.clear();
        quadVerts.clear();
    }
    NODISCARD bool empty() const { return triVerts.empty() && quadVerts.empty(); }
};

struct NODISCARD ConnectionMeshes final
{
    UniqueMesh normalTris;
    UniqueMesh redTris;
    UniqueMesh normalQuads;
    UniqueMesh redQuads;

    ConnectionMeshes() = default;
    DEFAULT_MOVES_DELETE_COPIES(ConnectionMeshes);
    ~ConnectionMeshes() = default;

    void render(int thisLayer, int focusedLayer) const;
};

struct NODISCARD ConnectionDrawerBuffers final
{
    ConnectionDrawerColorBuffer normal;
    ConnectionDrawerColorBuffer red;

    ConnectionDrawerBuffers() = default;
    DEFAULT_MOVES_DELETE_COPIES(ConnectionDrawerBuffers);
    ~ConnectionDrawerBuffers() = default;

    void clear()
    {
        normal.clear();
        red.clear();
    }

    NODISCARD bool empty() const { return red.empty() && normal.empty(); }
    NODISCARD ConnectionMeshes getMeshes(OpenGL &gl) const;
};

struct NODISCARD ConnectionDrawer final
{
private:
    struct NODISCARD ConnectionFakeGL final
    {
    private:
        ConnectionDrawerBuffers &m_buffers;
        ConnectionDrawerColorBuffer *m_currentBuffer = nullptr;
        glm::vec3 m_offset{0.f};

    public:
        explicit ConnectionFakeGL(ConnectionDrawerBuffers &buffers)
            : m_buffers{buffers}
            , m_currentBuffer{&m_buffers.normal}
        {}

        ~ConnectionFakeGL() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(ConnectionFakeGL);

    public:
        void setOffset(float x, float y, float z) { m_offset = glm::vec3{x, y, z}; }
        void setRed() { m_currentBuffer = &m_buffers.red; }
        void setNormal() { m_currentBuffer = &m_buffers.normal; }
        NODISCARD bool isNormal() const { return m_currentBuffer == &m_buffers.normal; }

    public:
        void drawTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);
        void drawLineStrip(const std::vector<glm::vec3> &points);
    };

private:
    ConnectionFakeGL m_fake;
    ConnectionDrawerBuffers &m_buffers;
    RoomNameBatch &m_roomNameBatch;
    const OptBounds &m_bounds;
    const int &m_currentLayer;

public:
    explicit ConnectionDrawer(ConnectionDrawerBuffers &buffers,
                              RoomNameBatch &roomNameBatch,
                              const int &currentLayer,
                              const OptBounds &bounds)
        : m_fake{buffers}
        , m_buffers{buffers}
        , m_roomNameBatch{roomNameBatch}
        , m_bounds{bounds}
        , m_currentLayer{currentLayer}
    {
        assert(m_buffers.empty());
        assert(m_roomNameBatch.empty());
    }

    ~ConnectionDrawer() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(ConnectionDrawer);

public:
    NODISCARD ConnectionFakeGL &getFakeGL() { return m_fake; }

    void drawRoomConnectionsAndDoors(const RoomHandle &room);

    void drawRoomDoorName(const RoomHandle &sourceRoom,
                          ExitDirEnum sourceDir,
                          const RoomHandle &targetRoom,
                          ExitDirEnum targetDir);

    void drawLineStrip(const std::vector<glm::vec3> &points);

    void drawConnection(const RoomHandle &leftRoom,
                        const RoomHandle &rightRoom,
                        ExitDirEnum startDir,
                        ExitDirEnum endDir,
                        bool oneway,
                        bool inExitFlags = true);

    void drawConnEndTriUpDownUnknown(float dX, float dY, float dstZ);

    void drawConnStartTri(ExitDirEnum startDir, float srcZ);

    void drawConnEndTri(ExitDirEnum endDir, float dX, float dY, float dstZ);

    void drawConnEndTri1Way(ExitDirEnum endDir, float dX, float dY, float dstZ);

    void drawConnectionLine(ExitDirEnum startDir,
                            ExitDirEnum endDir,
                            bool oneway,
                            bool neighbours,
                            float dX,
                            float dY,
                            float srcZ,
                            float dstZ);

    void drawConnectionTriangles(ExitDirEnum startDir,
                                 ExitDirEnum endDir,
                                 bool oneway,
                                 float dX,
                                 float dY,
                                 float srcZ,
                                 float dstZ);
};

using BatchedConnections = std::unordered_map<int, ConnectionDrawerBuffers>;
using BatchedConnectionMeshes = std::unordered_map<int, ConnectionMeshes>;
