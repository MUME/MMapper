#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <QString>

#include "../expandoracommon/coordinate.h"
#include "../global/Array.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "../opengl/Font.h"
#include "../opengl/OpenGLTypes.h"

class OpenGL;
class Room;

struct NODISCARD RoomNameBatch final
{
private:
    std::vector<GLText> m_names;

public:
    RoomNameBatch() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(RoomNameBatch);
    ~RoomNameBatch() = default;

public:
    void emplace_back(GLText &&glt) { m_names.emplace_back(std::move(glt)); }

public:
    void reserve(const size_t elements) { m_names.reserve(elements); }
    size_t size() const { return m_names.size(); }
    void clear() { m_names.clear(); }
    bool empty() const { return m_names.empty(); }

public:
    UniqueMesh getMesh(GLFont &font);
};

using BatchedRoomNames = std::unordered_map<int, UniqueMesh>;

struct NODISCARD ConnectionDrawerColorBuffer final
{
    std::vector<ColorVert> lineVerts;
    std::vector<ColorVert> triVerts;

    ConnectionDrawerColorBuffer() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(ConnectionDrawerColorBuffer);
    ~ConnectionDrawerColorBuffer() = default;

    void clear()
    {
        lineVerts.clear();
        triVerts.clear();
    }
    NODISCARD bool empty() const { return lineVerts.empty() && triVerts.empty(); }
};

struct NODISCARD ConnectionMeshes final
{
    UniqueMesh whiteLines;
    UniqueMesh whiteTris;
    UniqueMesh redLines;
    UniqueMesh redTris;

    ConnectionMeshes() = default;
    DEFAULT_MOVES_DELETE_COPIES(ConnectionMeshes);
    ~ConnectionMeshes() = default;

    void render(int thisLayer, int focusedLayer);
};

struct NODISCARD ConnectionDrawerBuffers final
{
    ConnectionDrawerColorBuffer white;
    ConnectionDrawerColorBuffer red;

    ConnectionDrawerBuffers() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(ConnectionDrawerBuffers);
    ~ConnectionDrawerBuffers() = default;

    void clear()
    {
        white.clear();
        red.clear();
    }
    NODISCARD bool empty() const { return red.empty() && white.empty(); }

    ConnectionMeshes getMeshes(OpenGL &gl);
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
        bool m_measureOnly = true;

        MMapper::Array<size_t, 2> m_expectedTriVerts;
        MMapper::Array<size_t, 2> m_expectedLineVerts;

    public:
        explicit ConnectionFakeGL(ConnectionDrawerBuffers &buffers)
            : m_buffers{buffers}
            , m_currentBuffer{&m_buffers.white}
        {}

        ~ConnectionFakeGL() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(ConnectionFakeGL);

    public:
        void endMeasurements()
        {
            m_measureOnly = false;

            assert(m_buffers.empty());
            m_buffers.white.lineVerts.reserve(m_expectedLineVerts[0]);
            m_buffers.white.triVerts.reserve(m_expectedTriVerts[0]);
            m_buffers.red.lineVerts.reserve(m_expectedLineVerts[1]);
            m_buffers.red.triVerts.reserve(m_expectedTriVerts[1]);
        }

        void verify()
        {
            assert(m_buffers.white.lineVerts.size() == m_expectedLineVerts[0]);
            assert(m_buffers.white.triVerts.size() == m_expectedTriVerts[0]);
            assert(m_buffers.red.lineVerts.size() == m_expectedLineVerts[1]);
            assert(m_buffers.red.triVerts.size() == m_expectedTriVerts[1]);
        }

    public:
        void setOffset(float x, float y, float z) { m_offset = glm::vec3{x, y, z}; }
        void setRed() { m_currentBuffer = &m_buffers.red; }
        void setWhite() { m_currentBuffer = &m_buffers.white; }
        bool isWhite() const { return m_currentBuffer == &m_buffers.white; }

    public:
        void drawTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
        {
            if (m_measureOnly) {
                m_expectedTriVerts[isWhite() ? 0 : 1] += 3;
                return;
            }

            const auto &color = isWhite() ? Colors::white : Colors::red;
            auto &verts = deref(m_currentBuffer).triVerts;
            verts.emplace_back(color, a + m_offset);
            verts.emplace_back(color, b + m_offset);
            verts.emplace_back(color, c + m_offset);
        }

        void drawLineStrip(const std::vector<glm::vec3> &points);
    };

private:
    ConnectionFakeGL m_fake;
    ConnectionDrawerBuffers &m_buffers;
    RoomNameBatch &m_roomNameBatch;
    const OptBounds &m_bounds;
    const int &m_currentLayer;
    bool m_measureOnly = true;
    size_t m_expectedRoomNames = 0;

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
    void endMeasurements()
    {
        m_measureOnly = false;
        m_fake.endMeasurements();
        m_roomNameBatch.reserve(m_expectedRoomNames);
    }

    void verify()
    {
        m_fake.verify();
        assert(m_roomNameBatch.size() == m_expectedRoomNames);
    }

    ConnectionFakeGL &getFakeGL() { return m_fake; }

    void drawRoomConnectionsAndDoors(const Room *room, const RoomIndex &rooms);

    void drawRoomDoorName(const Room *sourceRoom,
                          ExitDirEnum sourceDir,
                          const Room *targetRoom,
                          ExitDirEnum targetDir);

    void drawLineStrip(const std::vector<glm::vec3> &points);

    void drawConnection(const Room *leftRoom,
                        const Room *rightRoom,
                        ExitDirEnum startDir,
                        ExitDirEnum endDir,
                        bool oneway,
                        bool inExitFlags = true);

    void drawConnEndTriUpDownUnknown(int dX, int dY, float dstZ);

    void drawConnStartTri(ExitDirEnum startDir, float srcZ);

    void drawConnEndTri(ExitDirEnum endDir, int dX, int dY, float dstZ);

    void drawConnEndTri1Way(ExitDirEnum endDir, int dX, int dY, float dstZ);

    void drawConnectionLine(ExitDirEnum startDir,
                            ExitDirEnum endDir,
                            bool oneway,
                            bool neighbours,
                            int dX,
                            int dY,
                            float srcZ,
                            float dstZ);

    void drawConnectionTriangles(ExitDirEnum startDir,
                                 ExitDirEnum endDir,
                                 bool oneway,
                                 int dX,
                                 int dY,
                                 float srcZ,
                                 float dstZ);
};

using BatchedConnections = std::unordered_map<int, ConnectionDrawerBuffers>;
using BatchedConnectionMeshes = std::unordered_map<int, ConnectionMeshes>;
