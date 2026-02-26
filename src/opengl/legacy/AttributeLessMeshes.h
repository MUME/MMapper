#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../OpenGLTypes.h"
#include "Binders.h"
#include "Legacy.h"
#include "Shaders.h"
#include "VAO.h"

#include <memory>

namespace Legacy {

/**
 * @brief Base class for meshes that draw using gl_VertexID (no vertex attributes).
 */
template<typename ProgramType_>
class NODISCARD FullScreenMesh : public IRenderable
{
protected:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ProgramType_> m_shared_program;
    ProgramType_ &m_program;
    const GLenum m_mode;
    const GLsizei m_numVerts;

public:
    explicit FullScreenMesh(SharedFunctions sharedFunctions,
                            std::shared_ptr<ProgramType_> sharedProgram,
                            GLenum mode = GL_TRIANGLES,
                            GLsizei numVerts = 3)
        : m_shared_functions{std::move(sharedFunctions)}
        , m_functions{deref(m_shared_functions)}
        , m_shared_program{std::move(sharedProgram)}
        , m_program{deref(m_shared_program)}
        , m_mode(mode)
        , m_numVerts(numVerts)
    {}

    ~FullScreenMesh() override = default;

protected:
    void virt_clear() final {}
    void virt_reset() final {}
    NODISCARD bool virt_isEmpty() const final { return false; }

    void virt_render(const GLRenderState &renderState) override
    {
        auto binder = m_program.bind();
        // Attribute-less meshes usually don't use MVP, or use identity.
        // We pass identity as a default, but sub-classes can override.
        const glm::mat4 identity = glm::mat4(1.0f);
        m_program.setUniforms(identity, renderState.uniforms);

        RenderStateBinder rsBinder(m_functions, m_functions.getTexLookup(), renderState);

        SharedVao shared = m_functions.getSharedVaos().get(SharedVaoEnum::EmptyVao);
        VAO &vao = deref(shared);
        if (!vao) {
            vao.emplace(m_shared_functions);
        }

        m_functions.glBindVertexArray(vao.get());
        m_functions.glDrawArrays(m_mode, 0, m_numVerts);
        m_functions.glBindVertexArray(0);
    }
};

class NODISCARD BlitMesh final : public FullScreenMesh<BlitShader>
{
public:
    using FullScreenMesh::FullScreenMesh;
    ~BlitMesh() override;
};

class NODISCARD PlainFullScreenMesh final : public FullScreenMesh<FullScreenShader>
{
public:
    using FullScreenMesh::FullScreenMesh;
    ~PlainFullScreenMesh() override;
};

} // namespace Legacy
