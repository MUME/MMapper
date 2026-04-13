#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/View.h"
#include "Meshes.h"
#include "Shaders.h"
#include "SimpleMesh.h"

#include <memory>
#include <optional>

#define VPO(_x) offsetof(VertexType_, _x)
#define CHECK_ATTR_LOC(_expect, _name) assert(Base::m_program.getAttribLocation(_name) == (_expect))

namespace Legacy {

// Textured mesh with color modulated by color attribute,
// but it does a screen space transform. See FontVert3d.
template<typename VertexType_>
class NODISCARD SimpleFont3dMesh : public SimpleMesh<VertexType_, FontShader>
{
public:
    using Base = SimpleMesh<VertexType_, FontShader>;

public:
    explicit SimpleFont3dMesh(const SharedFunctions &sharedFunctions,
                              const std::shared_ptr<FontShader> &sharedProgram,
                              DoNotAllocateVao)
        : Base(sharedFunctions, sharedProgram, DoNotAllocateVao{})
    {}
    explicit SimpleFont3dMesh(const SharedFunctions &sharedFunctions,
                              const std::shared_ptr<FontShader> &sharedProgram,
                              const DrawModeEnum mode,
                              const View<VertexType_> verts)
        : Base(sharedFunctions, sharedProgram, mode, verts)
    {}

private:
    void virt_setupAttribs() final
    {
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        static_assert(sizeof(std::declval<VertexType_>().base) == 3 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().color) == 4 * sizeof(uint8_t));
        static_assert(sizeof(std::declval<VertexType_>().tex) == 2 * sizeof(GLfloat));
        static_assert(sizeof(std::declval<VertexType_>().vert) == 2 * sizeof(GLfloat));

        Functions &gl = Base::m_functions;
        CHECK_ATTR_LOC(0, "aBase");
        CHECK_ATTR_LOC(1, "aColor");
        CHECK_ATTR_LOC(2, "aTexCoord");
        CHECK_ATTR_LOC(3, "aVert");

        gl.enableAttrib(0, 3, GL_FLOAT, GL_FALSE, vertSize, VPO(base));
        gl.enableAttrib(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertSize, VPO(color));
        gl.enableAttrib(2, 2, GL_FLOAT, GL_FALSE, vertSize, VPO(tex));
        gl.enableAttrib(3, 2, GL_FLOAT, GL_FALSE, vertSize, VPO(vert));
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
                        View<FontVert3d> verts);

public:
    ~FontMesh3d() final;

private:
    NODISCARD bool virt_modifiesRenderState() const final { return true; }
    NODISCARD GLRenderState virt_modifyRenderState(const GLRenderState &renderState) const final;
};

} // namespace Legacy

#undef VPO
#undef CHECK_ATTR_LOC
