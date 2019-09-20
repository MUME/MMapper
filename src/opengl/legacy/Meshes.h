#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/utils.h"
#include "Legacy.h"
#include "Shaders.h"
#include "SimpleMesh.h"

#define VOIDPTR_OFFSETOF(x, y) reinterpret_cast<void *>(offsetof(x, y))
#define VPO(x) VOIDPTR_OFFSETOF(_VertexType, x)

namespace Legacy {

// Uniform color
template<typename _VertexType>
class NODISCARD PlainMesh final : public SimpleMesh<_VertexType, UColorPlainShader>
{
public:
    using Base = SimpleMesh<_VertexType, UColorPlainShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> boundAttribs;

    void virt_bind() override
    {
        static_assert(sizeof(std::declval<_VertexType>()) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        boundAttribs.reset();
    }
};

// Per-vertex color
// flat-shaded in MMapper, due to glShadeModel(GL_FLAT)
template<typename _VertexType>
class NODISCARD ColoredMesh final : public SimpleMesh<_VertexType, AColorPlainShader>
{
public:
    using Base = SimpleMesh<_VertexType, AColorPlainShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(_VertexType));
        static_assert(sizeof(std::declval<_VertexType>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<_VertexType>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        boundAttribs.reset();
    }
}; // namespace Legacy

// Textured mesh with color modulated by uniform
template<typename _VertexType>
class NODISCARD TexturedMesh final : public SimpleMesh<_VertexType, UColorTexturedShader>
{
public:
    using Base = SimpleMesh<_VertexType, UColorTexturedShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint texPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.texPos = fontShader.getAttribLocation("aTexCoord");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(_VertexType));
        static_assert(sizeof(std::declval<_VertexType>().tex) == 2 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<_VertexType>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        /* NOTE: OpenGL 2.x can't use GL_TEXTURE_2D_ARRAY. */
        gl.enableAttrib(attribs.texPos, 2, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.texPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        boundAttribs.reset();
    }
};

// Textured mesh with color modulated by color attribute.
template<typename _VertexType>
class NODISCARD ColoredTexturedMesh final : public SimpleMesh<_VertexType, AColorTexturedShader>
{
public:
    using Base = SimpleMesh<_VertexType, AColorTexturedShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint texPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        static Attribs getLocations(AColorTexturedShader &fontShader)
        {
            Attribs result;
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.texPos = fontShader.getAttribLocation("aTexCoord");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(_VertexType));
        static_assert(sizeof(std::declval<_VertexType>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<_VertexType>().tex) == 2 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<_VertexType>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        /* NOTE: OpenGL 2.x can't use GL_TEXTURE_2D_ARRAY. */
        gl.enableAttrib(attribs.texPos, 2, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.texPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        boundAttribs.reset();
    }
};

// Per-vertex color
// flat-shaded in MMapper, due to glShadeModel(GL_FLAT)
template<typename _VertexType>
class NODISCARD PointMesh final : public SimpleMesh<_VertexType, PointShader>
{
public:
    using Base = SimpleMesh<_VertexType, PointShader>;
    using Base::Base;

private:
    struct NODISCARD Attribs final
    {
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(_VertexType));
        static_assert(sizeof(std::declval<_VertexType>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<_VertexType>().vert) == 3 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(attribs.vertPos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!boundAttribs) {
            assert(false);
            return;
        }

        auto &attribs = boundAttribs.value();
        Functions &gl = Base::m_functions;
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        boundAttribs.reset();
    }
};

} // namespace Legacy

#undef VOIDPTR_OFFSETOF
#undef VPO
