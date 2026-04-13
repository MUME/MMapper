#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/utils.h"
#include "Legacy.h"
#include "Shaders.h"
#include "SimpleMesh.h"

#include <optional>

#define VPO(x) offsetof(VertexType_, x)
#define CHECK_ATTR_LOC(_expect, _name) assert(Base::m_program.getAttribLocation(_name) == (_expect))

namespace Legacy {

// Uniform color
template<typename VertexType_>
class NODISCARD PlainMesh final : public SimpleMesh<VertexType_, UColorPlainShader>
{
public:
    using Base = SimpleMesh<VertexType_, UColorPlainShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        static_assert(sizeof(std::declval<VertexType_>()) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aVert");
        gl.enableAttrib(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }
};

template<typename VertexType_>
class NODISCARD LineMesh final : public SimpleMeshBase<VertexType_, UColorLineShader>
{
public:
    using Base = SimpleMeshBase<VertexType_, UColorLineShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        Functions &gl = Base::m_functions;

        CHECK_ATTR_LOC(0, "aVert1");
        CHECK_ATTR_LOC(1, "aVert2");

        const auto stride = 2 * vertSize;
        gl.enableAttrib(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
        gl.enableAttrib(1, 3, GL_FLOAT, GL_FALSE, stride, vertSize);

        // instancing: one line per 4 vertices (quad)
        gl.glVertexAttribDivisor(0, 1);
        gl.glVertexAttribDivisor(1, 1);
    }

private:
    void virt_setCustomUniforms(Functions &gl, const GLRenderState &renderState) final
    {
        auto &prog = Base::m_program;
        prog.setViewport("uViewport", gl.getPhysicalViewport());
        prog.setFloat("uLineWidth", renderState.lineParams.width * gl.getDevicePixelRatio());
    }

private:
    void virt_draw(Functions &gl) final
    {
        const GLsizei numInstances = Base::m_numVerts / 2;
        gl.glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, numInstances);
    }
};

// Per-vertex color
// flat-shaded in MMapper, due to glShadeModel(GL_FLAT)
template<typename VertexType_>
class NODISCARD ColoredMesh final : public SimpleMesh<VertexType_, AColorPlainShader>
{
public:
    using Base = SimpleMesh<VertexType_, AColorPlainShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aColor");
        CHECK_ATTR_LOC(1, "aVert");
        gl.enableAttrib(0, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(1, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
    }
};

template<typename VertexType_>
class NODISCARD ColoredLineMesh final : public SimpleMeshBase<VertexType_, AColorLineShader>
{
public:
    using Base = SimpleMeshBase<VertexType_, AColorLineShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        Functions &gl = Base::m_functions;

        CHECK_ATTR_LOC(0, "aColor");
        CHECK_ATTR_LOC(1, "aVert1");
        CHECK_ATTR_LOC(2, "aVert2");

        const auto stride = 2 * vertSize;
        // We assume the color comes from the first vertex of the segment
        gl.enableAttrib(0, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, VPO(color));
        gl.enableAttrib(1, 3, GL_FLOAT, GL_FALSE, stride, VPO(vert));
        gl.enableAttrib(2, 3, GL_FLOAT, GL_FALSE, stride, VPO(vert) + vertSize);

        // instancing
        gl.glVertexAttribDivisor(0, 1);
        gl.glVertexAttribDivisor(1, 1);
        gl.glVertexAttribDivisor(2, 1);
    }

private:
    void virt_setCustomUniforms(Functions &gl, const GLRenderState &renderState) final
    {
        auto &prog = Base::m_program;
        prog.setViewport("uViewport", gl.getPhysicalViewport());
        prog.setFloat("uLineWidth", renderState.lineParams.width * gl.getDevicePixelRatio());
    }

private:
    void virt_draw(Functions &gl) final
    {
        const GLsizei numInstances = Base::m_numVerts / 2;
        gl.glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, numInstances);
    }
};

// Textured mesh with color modulated by uniform
template<typename VertexType_>
class NODISCARD TexturedMesh final : public SimpleMesh<VertexType_, UColorTexturedShader>
{
public:
    using Base = SimpleMesh<VertexType_, UColorTexturedShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().tex) == 3 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aTexCoord");
        CHECK_ATTR_LOC(1, "aVert");
        gl.enableAttrib(0, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(1, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
    }
};

// Textured mesh with color modulated by color attribute.
template<typename VertexType_>
class NODISCARD ColoredTexturedMesh final : public SimpleMesh<VertexType_, AColorTexturedShader>
{
public:
    using Base = SimpleMesh<VertexType_, AColorTexturedShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().tex) == 3 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aColor");
        CHECK_ATTR_LOC(1, "aTexCoord");
        CHECK_ATTR_LOC(2, "aVert");
        gl.enableAttrib(0, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(1, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(2, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
    }
};

// Textured mesh with color modulated by color attribute.
template<typename VertexType_>
class NODISCARD RoomQuadTexMesh final : public SimpleMesh<VertexType_, RoomQuadTexShader>
{
public:
    using Base = SimpleMesh<VertexType_, RoomQuadTexShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().vertTexCol) == 4 * sizeof(int32_t));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aVertTexCol");
        // ivec4
        gl.enableAttribI(0, 4, GL_INT, vertSize, VPO(vertTexCol));
        // instancing
        gl.glVertexAttribDivisor(0, 1);
    }
};

// Per-vertex color
// flat-shaded in MMapper, due to glShadeModel(GL_FLAT)
template<typename VertexType_>
class NODISCARD PointMesh final : public SimpleMesh<VertexType_, PointShader>
{
public:
    using Base = SimpleMesh<VertexType_, PointShader>;
    using Base::Base;

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aColor");
        CHECK_ATTR_LOC(1, "aVert");
        gl.enableAttrib(0, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(1, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
    }
};

} // namespace Legacy

#undef VPO
#undef CHECK_ATTR_LOC
