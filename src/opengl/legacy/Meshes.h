#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/utils.h"
#include "Legacy.h"
#include "Shaders.h"
#include "SimpleMesh.h"

#include <optional>

#define VOIDPTR_OFFSETOF(x, y) reinterpret_cast<void *>(offsetof(x, y))
#define VPO(x) VOIDPTR_OFFSETOF(VertexType_, x)

namespace Legacy {

// Uniform color
template<typename VertexType_>
class NODISCARD PlainMesh final : public SimpleMesh<VertexType_, UColorPlainShader>
{
public:
    using Base = SimpleMesh<VertexType_, UColorPlainShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AbstractShaderProgram &shader)
        {
            Attribs result;
            result.vertPos = shader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        static_assert(sizeof(std::declval<VertexType_>()) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        m_boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
    }
};

template<typename VertexType_>
class NODISCARD LineMesh final : public SimpleMesh<VertexType_, UColorLineShader>
{
public:
    using Base = SimpleMesh<VertexType_, UColorLineShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint vert1Pos = INVALID_ATTRIB_LOCATION;
        GLuint vert2Pos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AbstractShaderProgram &shader)
        {
            Attribs result;
            result.vert1Pos = shader.getAttribLocation("aVert1");
            result.vert2Pos = shader.getAttribLocation("aVert2");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        // aVert1
        gl.enableAttrib(attribs.vert1Pos, 3, GL_FLOAT, GL_FALSE, 2 * vertSize, nullptr);
        // aVert2
        gl.enableAttrib(attribs.vert2Pos, 3, GL_FLOAT, GL_FALSE, 2 * vertSize, VPO_PTR(vertSize));

        // instancing: one line per 4 vertices (quad)
        gl.glVertexAttribDivisor(attribs.vert1Pos, 1);
        gl.glVertexAttribDivisor(attribs.vert2Pos, 1);

        m_boundAttribs = attribs;
    }

    void virt_render(const GLRenderState &renderState) override
    {
        if (Base::isEmpty()) {
            return;
        }

        Functions &gl = Base::m_functions;
        gl.checkError();

        const glm::mat4 mvp = renderState.mvp.value_or(gl.getProjectionMatrix());
        auto programUnbinder = Base::m_program.bind();
        Base::m_program.setUniforms(mvp, renderState.uniforms);
        Base::m_program.setViewport("uViewport", gl.getPhysicalViewport());
        Base::m_program.setFloat("uLineWidth",
                                 renderState.lineParams.width * gl.getDevicePixelRatio());

        RenderStateBinder renderStateBinder(gl, gl.getTexLookup(), renderState);

        gl.glBindVertexArray(Base::m_vao.get());
        RAIICallback vaoUnbinder([&gl]() {
            gl.glBindVertexArray(0);
            gl.checkError();
        });

        auto attribUnbinder = Base::bindAttribs();

        const GLsizei numInstances = Base::m_numVerts / 2;
        gl.glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, numInstances);
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            return;
        }
        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.vert1Pos);
        gl.glDisableVertexAttribArray(attribs.vert2Pos);
        gl.glVertexAttribDivisor(attribs.vert1Pos, 0);
        gl.glVertexAttribDivisor(attribs.vert2Pos, 0);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
    }

    NODISCARD static const void *VPO_PTR(size_t offset)
    {
        return reinterpret_cast<const void *>(offset);
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
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        m_boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
    }
};

template<typename VertexType_>
class NODISCARD ColoredLineMesh final : public SimpleMesh<VertexType_, AColorLineShader>
{
public:
    using Base = SimpleMesh<VertexType_, AColorLineShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint vert1Pos = INVALID_ATTRIB_LOCATION;
        GLuint vert2Pos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AbstractShaderProgram &shader)
        {
            Attribs result;
            result.colorPos = shader.getAttribLocation("aColor");
            result.vert1Pos = shader.getAttribLocation("aVert1");
            result.vert2Pos = shader.getAttribLocation("aVert2");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());

        // We assume the color comes from the first vertex of the segment
        // aColor
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, 2 * vertSize, VPO(color));
        // aVert1
        gl.enableAttrib(attribs.vert1Pos, 3, GL_FLOAT, GL_FALSE, 2 * vertSize, VPO(vert));
        // aVert2
        gl.enableAttrib(attribs.vert2Pos,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        2 * vertSize,
                        VPO_PTR(VPO(vert), vertSize));

        // instancing
        gl.glVertexAttribDivisor(attribs.colorPos, 1);
        gl.glVertexAttribDivisor(attribs.vert1Pos, 1);
        gl.glVertexAttribDivisor(attribs.vert2Pos, 1);

        m_boundAttribs = attribs;
    }

    void virt_render(const GLRenderState &renderState) override
    {
        if (Base::isEmpty()) {
            return;
        }

        Functions &gl = Base::m_functions;
        gl.checkError();

        const glm::mat4 mvp = renderState.mvp.value_or(gl.getProjectionMatrix());
        auto programUnbinder = Base::m_program.bind();
        Base::m_program.setUniforms(mvp, renderState.uniforms);
        Base::m_program.setViewport("uViewport", gl.getPhysicalViewport());
        Base::m_program.setFloat("uLineWidth",
                                 renderState.lineParams.width * gl.getDevicePixelRatio());

        RenderStateBinder renderStateBinder(gl, gl.getTexLookup(), renderState);

        gl.glBindVertexArray(Base::m_vao.get());
        RAIICallback vaoUnbinder([&gl]() {
            gl.glBindVertexArray(0);
            gl.checkError();
        });

        auto attribUnbinder = Base::bindAttribs();

        const GLsizei numInstances = Base::m_numVerts / 2;
        gl.glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, numInstances);
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            return;
        }
        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.vert1Pos);
        gl.glDisableVertexAttribArray(attribs.vert2Pos);
        gl.glVertexAttribDivisor(attribs.colorPos, 0);
        gl.glVertexAttribDivisor(attribs.vert1Pos, 0);
        gl.glVertexAttribDivisor(attribs.vert2Pos, 0);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
    }

    NODISCARD static const void *VPO_PTR(const void *base, size_t offset)
    {
        return static_cast<const char *>(base) + offset;
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
    struct NODISCARD Attribs final
    {
        GLuint texPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.texPos = fontShader.getAttribLocation("aTexCoord");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().tex) == 3 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.texPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        m_boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.texPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
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
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint texPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AColorTexturedShader &fontShader)
        {
            Attribs result;
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.texPos = fontShader.getAttribLocation("aTexCoord");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().tex) == 3 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(attribs.texPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        m_boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.texPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
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
    struct NODISCARD Attribs final
    {
        GLuint vertTexColPos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(RoomQuadTexShader &fontShader)
        {
            Attribs result;
            result.vertTexColPos = fontShader.getAttribLocation("aVertTexCol");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().vertTexCol) == 4 * sizeof(int32_t));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        // ivec4
        gl.enableAttribI(attribs.vertTexColPos, 4, GL_INT, vertSize, VPO(vertTexCol));

        // instancing
        gl.glVertexAttribDivisor(attribs.vertTexColPos, 1);

        m_boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.vertTexColPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
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
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        NODISCARD static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> m_boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        m_boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!m_boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = m_boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_boundAttribs.reset();
    }
};

} // namespace Legacy

#undef VOIDPTR_OFFSETOF
#undef VPO
