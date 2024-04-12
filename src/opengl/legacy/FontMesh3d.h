#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <memory>
#include <optional>
#include <vector>

#include "Shaders.h"
#include "SimpleMesh.h"

#define VOIDPTR_OFFSETOF(x, y) reinterpret_cast<void *>(offsetof(x, y))
#define VPO(x) VOIDPTR_OFFSETOF(VertexType_, x)

namespace Legacy {

// Textured mesh with color modulated by color attribute,
// but it does a screen space transform. See FontVert3d.
template<typename VertexType_>
class NODISCARD SimpleFont3dMesh : public SimpleMesh<VertexType_, FontShader>
{
public:
    using Base = SimpleMesh<VertexType_, FontShader>;

    explicit SimpleFont3dMesh(const SharedFunctions &sharedFunctions,
                              const std::shared_ptr<FontShader> &sharedProgram)
        : Base(sharedFunctions, sharedProgram)
    {}

    explicit SimpleFont3dMesh(const SharedFunctions &sharedFunctions,
                              const std::shared_ptr<FontShader> &sharedProgram,
                              const DrawModeEnum mode,
                              const std::vector<VertexType_> &verts)
        : Base(sharedFunctions, sharedProgram, mode, verts)
    {}

private:
    struct NODISCARD Attribs final
    {
        GLuint basePos = INVALID_ATTRIB_LOCATION;
        GLuint colorPos = INVALID_ATTRIB_LOCATION;
        GLuint texPos = INVALID_ATTRIB_LOCATION;
        GLuint vertPos = INVALID_ATTRIB_LOCATION;

        static Attribs getLocations(AbstractShaderProgram &fontShader)
        {
            Attribs result;
            result.basePos = fontShader.getAttribLocation("aBase");
            result.colorPos = fontShader.getAttribLocation("aColor");
            result.texPos = fontShader.getAttribLocation("aTexCoord");
            result.vertPos = fontShader.getAttribLocation("aVert");
            return result;
        }
    };

    std::optional<Attribs> boundAttribs;

    void virt_bind() override
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().base) == 3 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().tex) == 2 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 2 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;

        gl.glBindBuffer(GL_ARRAY_BUFFER, Base::m_vbo.get());
        const auto attribs = Attribs::getLocations(Base::m_program);
        gl.enableAttrib(attribs.basePos, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(base));
        gl.enableAttrib(attribs.colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(attribs.texPos, 2, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(attribs.vertPos, 2, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
        boundAttribs = attribs;
    }

    void virt_unbind() override
    {
        if (!boundAttribs) {
            assert(false);
            return;
        }
        Functions &gl = Base::m_functions;
        const auto attribs = boundAttribs.value();
        gl.glDisableVertexAttribArray(attribs.basePos);
        gl.glDisableVertexAttribArray(attribs.colorPos);
        gl.glDisableVertexAttribArray(attribs.texPos);
        gl.glDisableVertexAttribArray(attribs.vertPos);
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        boundAttribs.reset();
    }
};

class NODISCARD FontMesh3d final : public SimpleFont3dMesh<FontVert3d>
{
private:
    using Base = SimpleFont3dMesh<FontVert3d>;
    SharedMMTexture m_texture;

public:
    explicit FontMesh3d(const SharedFunctions &functions,
                        const std::shared_ptr<FontShader> &sharedShader,
                        SharedMMTexture texture,
                        DrawModeEnum mode,
                        const std::vector<FontVert3d> &verts);

public:
    ~FontMesh3d() final;

private:
    NODISCARD bool virt_modifiesRenderState() const final { return true; }
    NODISCARD GLRenderState virt_modifyRenderState(const GLRenderState &renderState) const final;
};

} // namespace Legacy

#undef VOIDPTR_OFFSETOF
#undef VPO
