#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/utils.h"
#include "../map/coordinate.h"
#include "../opengl/Font.h"
#include "../opengl/OpenGLTypes.h"

#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <stack>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QColor>

class MapScreen;
class OpenGL;
struct MapCanvasTextures;

// TODO: find a better home for this. It's common to characters and room selections.
class NODISCARD DistantObjectTransform final
{
public:
    const glm::vec3 offset{};
    // rotation counterclockwise around the Z axis, starting at the +X axis.
    const float rotationDegrees = 0.f;

public:
    explicit DistantObjectTransform(const glm::vec3 &offset_, const float rotationDegrees_)
        : offset{offset_}
        , rotationDegrees{rotationDegrees_}
    {}

public:
    // Caller must apply the correct translation and rotation.
    NODISCARD static DistantObjectTransform construct(const glm::vec3 &pos,
                                                      const MapScreen &mapScreen,
                                                      float marginPixels);
};

class NODISCARD CharacterBatch final
{
private:
    static constexpr const float FILL_ALPHA = 0.1f;
    static constexpr const float BEACON_ALPHA = 0.10f;
    static constexpr const float LINE_ALPHA = 0.9f;

private:
    struct NODISCARD Matrices final
    {
        // glm::mat4 proj = glm::mat4(1);
        glm::mat4 modelView = glm::mat4(1);
    };

    class NODISCARD MatrixStack final : private std::stack<Matrices>
    {
    private:
        using base = std::stack<Matrices>;

    public:
        MatrixStack() { base::push(Matrices()); }
        ~MatrixStack() { assert(base::size() == 1); }
        void push() { base::push(top()); }
        using base::pop;
        using base::top;
    };

    class NODISCARD CharFakeGL final
    {
    private:
        struct NODISCARD CoordCompare final
        {
            // REVISIT: just make this the global operator<(),
            // so it can be picked up by std::less<Coordinate>.
            bool operator()(const Coordinate &lhs, const Coordinate &rhs) const
            {
                if (lhs.x < rhs.x) {
                    return true;
                }
                if (rhs.x < lhs.x) {
                    return false;
                }
                if (lhs.y < rhs.y) {
                    return true;
                }
                if (rhs.y < lhs.y) {
                    return false;
                }
                return lhs.z < rhs.z;
            }
        };

    private:
        Color m_color;
        MatrixStack m_stack;
        std::vector<ColorVert> m_charTris;
        std::vector<ColorVert> m_charBeaconQuads;
        std::vector<ColoredTexVert> m_charTokenQuads;
        std::vector<QString> m_charTokenKeys;
        std::vector<ColorVert> m_charLines;
        std::vector<ColorVert> m_pathPoints;
        std::vector<ColorVert> m_pathLineVerts;
        std::vector<ColoredTexVert> m_charRoomQuads;
        std::vector<FontVert3d> m_screenSpaceArrows;
        std::map<Coordinate, int, CoordCompare> m_coordCounts;

    public:
        CharFakeGL() = default;
        ~CharFakeGL() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(CharFakeGL);

    public:
        void reallyDraw(OpenGL &gl, const MapCanvasTextures &textures)
        {
            reallyDrawCharacters(gl, textures);
            reallyDrawPaths(gl);
        }

    private:
        void reallyDrawCharacters(OpenGL &gl, const MapCanvasTextures &textures);
        void reallyDrawPaths(OpenGL &gl);

    public:
        void setColor(const Color &color) { m_color = color; }
        void reserve(Coordinate c) { m_coordCounts[c]++; }
        void clear(Coordinate c) { m_coordCounts[c] = 0; }

    public:
        void glPushMatrix() { m_stack.push(); }
        void glPopMatrix() { m_stack.pop(); }
        void glRotateZ(float degrees)
        {
            auto &m = m_stack.top().modelView;
            m = glm::rotate(m, glm::radians(degrees), glm::vec3{0, 0, 1});
        }
        void glScalef(float x, float y, float z)
        {
            auto &m = m_stack.top().modelView;
            m = glm::scale(m, glm::vec3{x, y, z});
        }
        void glTranslatef(const glm::vec3 &v)
        {
            auto &m = m_stack.top().modelView;
            m = glm::translate(m, v);
        }
        void drawArrow(bool fill, bool beacon);
        void drawBox(
            const Coordinate &coord, bool fill, bool beacon, bool isFar, const QString &dispName);
        void addScreenSpaceArrow(const glm::vec3 &pos, float degrees, const Color &color, bool fill);

        // with blending, without depth; always size 4
        void drawPathLineStrip(const Color &color, const std::vector<glm::vec3> &points)
        {
            for (size_t i = 1, size = points.size(); i < size; ++i) {
                m_pathLineVerts.emplace_back(color, points[i - 1]);
                m_pathLineVerts.emplace_back(color, points[i]);
            }
        }

        // with blending, without depth; always size 8
        void drawPathPoint(const Color &color, const glm::vec3 &pos)
        {
            m_pathPoints.emplace_back(color, pos);
        }

    private:
        enum class NODISCARD QuadOptsEnum : uint32_t {
            NONE = 0,
            OUTLINE = 1,
            FILL = 2,
            BEACON = 4
        };
        NODISCARD friend QuadOptsEnum operator|(const QuadOptsEnum lhs, const QuadOptsEnum rhs)
        {
            return static_cast<QuadOptsEnum>(static_cast<uint32_t>(lhs)
                                             | static_cast<uint32_t>(rhs));
        }
        NODISCARD friend QuadOptsEnum operator&(const QuadOptsEnum lhs, const QuadOptsEnum rhs)
        {
            return static_cast<QuadOptsEnum>(static_cast<uint32_t>(lhs)
                                             & static_cast<uint32_t>(rhs));
        }
        void drawQuadCommon(const glm::vec2 &a,
                            const glm::vec2 &b,
                            const glm::vec2 &c,
                            const glm::vec2 &d,
                            QuadOptsEnum options);
    };

private:
    const MapScreen &m_mapScreen;
    const int m_currentLayer;
    const float m_scale;
    CharFakeGL m_fakeGL;

public:
    explicit CharacterBatch(const MapScreen &mapScreen, const int currentLayer, const float scale)
        : m_mapScreen(mapScreen)
        , m_currentLayer(currentLayer)
        , m_scale(scale)
    {}

protected:
    NODISCARD CharFakeGL &getOpenGL() { return m_fakeGL; }

public:
    void incrementCount(const Coordinate &c) { getOpenGL().reserve(c); }

    void resetCount(const Coordinate &c) { getOpenGL().clear(c); }

    NODISCARD bool isVisible(const Coordinate &c, float margin) const;

public:
    void drawCharacter(const Coordinate &coordinate,
                       const Color &color,
                       bool fill = true,
                       const QString &dispName = QString());

    void drawPreSpammedPath(const Coordinate &coordinate,
                            const std::vector<Coordinate> &path,
                            const Color &color);

public:
    void reallyDraw(OpenGL &gl, const MapCanvasTextures &textures)
    {
        m_fakeGL.reallyDraw(gl, textures);
    }
};
