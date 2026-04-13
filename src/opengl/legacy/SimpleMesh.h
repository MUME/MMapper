#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/RAII.h"
#include "../../global/View.h"
#include "../OpenGLTypes.h"
#include "AbstractShaderProgram.h"
#include "Binders.h"
#include "Legacy.h"
#include "VAO.h"
#include "VBO.h"

#include <cassert>
#include <memory>
#include <optional>

namespace Legacy {

void drawRoomQuad(Functions &gl, GLsizei numVerts);

struct NODISCARD DoNotAllocateVao final
{
    explicit DoNotAllocateVao() = default;
};

template<typename VertexType_, typename ProgramType_>
class NODISCARD SimpleMeshBase : public IRenderable
{
public:
    using ProgramType = ProgramType_;
    static_assert(std::is_base_of_v<AbstractShaderProgram, ProgramType>);

private:
    static constexpr bool LOG_VBO_UPLOADS = false;

protected:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ProgramType_> m_shared_program;
    ProgramType_ &m_program;
    VBO m_vbo;
    VAO m_vao;
    DrawModeEnum m_drawMode = DrawModeEnum::INVALID;
    GLsizei m_numVerts = 0;

private:
    bool m_vaoConfigured = false;

public:
    explicit SimpleMeshBase(SharedFunctions sharedFunctions,
                            std::shared_ptr<ProgramType_> sharedProgram,
                            const std::optional<DoNotAllocateVao> noVao)
        : m_shared_functions{std::move(sharedFunctions)}
        , m_functions{deref(m_shared_functions)}
        , m_shared_program{std::move(sharedProgram)}
        , m_program{deref(m_shared_program)}
    {
        if (!noVao) {
            m_vao.emplace(m_shared_functions);
        }
    }

    explicit SimpleMeshBase(const SharedFunctions &sharedFunctions,
                            const std::shared_ptr<ProgramType_> &sharedProgram,
                            const DrawModeEnum mode,
                            const View<VertexType_> verts)
        : SimpleMeshBase{sharedFunctions, sharedProgram, std::nullopt}
    {
        setStatic(mode, verts);
    }

    ~SimpleMeshBase() override { reset(); }

private:
    virtual void virt_setupAttribs() = 0;

    // Records vertex attribute state in the VAO so it doesn't need to be
    // re-established on every draw call.
    void setupVaoAttribs()
    {
        assert(m_vao.isValid() && m_vbo.isValid());
        MAYBE_UNUSED auto vbo_binder = m_vbo.bind(GL_ARRAY_BUFFER);
        MAYBE_UNUSED auto vao_binder = m_vao.bind();
        virt_setupAttribs();
        m_vaoConfigured = true;
    }

private:
    void trySetupVaoAttribs()
    {
        if (!m_vaoConfigured) {
            setupVaoAttribs();
        }
    }

public:
    void unsafe_swapVaoId(VAO &vao) { return m_vao.unsafe_swapVaoId(vao); }
    void unsafe_swapVboId(VBO &vbo) { return m_vbo.unsafe_swapVboId(vbo); }

public:
    void setDynamic(const DrawModeEnum mode, const View<VertexType_> verts)
    {
        setCommon(mode, verts, BufferUsageEnum::DYNAMIC_DRAW);
    }
    void setStatic(const DrawModeEnum mode, const View<VertexType_> verts)
    {
        setCommon(mode, verts, BufferUsageEnum::STATIC_DRAW);
    }

private:
    void setCommon(const DrawModeEnum mode,
                   const View<VertexType_> verts,
                   const BufferUsageEnum usage)
    {
        const auto numVerts = verts.size();

        static_assert(static_cast<size_t>(DrawModeEnum::POINTS) == 1);
        static_assert(static_cast<size_t>(DrawModeEnum::LINES) == 2);
        static_assert(static_cast<size_t>(DrawModeEnum::TRIANGLES) == 3);
        static_assert(static_cast<size_t>(DrawModeEnum::QUADS) == 4);
        assert(mode == DrawModeEnum::INVALID || mode == DrawModeEnum::INSTANCED_QUADS
               || numVerts % static_cast<size_t>(mode) == 0);

        if (!m_vbo.isValid() && numVerts != 0) {
            m_vbo.emplace(m_shared_functions);
            m_vaoConfigured = false;
        }

        if (LOG_VBO_UPLOADS && usage == BufferUsageEnum::STATIC_DRAW && m_vbo.isValid()) {
            qInfo() << "Uploading static buffer with" << numVerts << "verts of size"
                    << sizeof(VertexType_) << "(total" << (numVerts * sizeof(VertexType_))
                    << "bytes) to VBO" << m_vbo.get() << __FUNCTION__;
        }

        if (m_vbo.isValid()) {
            auto tmp = m_vbo.setVbo(mode, verts, usage);
            m_drawMode = tmp.first;
            m_numVerts = tmp.second;
        } else {
            // REVISIT: Should this be reported as an error?
            m_drawMode = DrawModeEnum::INVALID;
            m_numVerts = 0;
        }

        if (!m_vao.isValid() || !m_vbo.isValid()) {
            m_vaoConfigured = false;
        }
    }

private:
    // Clears the contents of the mesh, but does not give up its GL resources.
    void virt_clear() final
    {
        if (m_drawMode != DrawModeEnum::INVALID) {
            setStatic(m_drawMode, {});
        }
        assert(isEmpty());
    }

    // Clears the mesh and destroys the GL resources.
    void virt_reset() final
    {
        m_vao.reset();
        m_drawMode = DrawModeEnum::INVALID;
        m_numVerts = 0;
        m_vbo.reset();
        m_vaoConfigured = false;
        assert(isEmpty() && !m_vbo.isValid());
    }

private:
    NODISCARD bool virt_isEmpty() const final
    {
        return !m_vbo.isValid() || m_numVerts == 0 || m_drawMode == DrawModeEnum::INVALID;
    }

private:
    virtual void virt_setCustomUniforms(Functions &gl, const GLRenderState &renderState) = 0;
    virtual void virt_draw(Functions &gl) = 0;

private:
    void virt_render(const GLRenderState &renderState) final
    {
        if (isEmpty()) {
            return;
        }

        auto &gl = m_functions;
        gl.checkError();

        const glm::mat4 mvp = renderState.mvp.value_or(gl.getProjectionMatrix());
        auto &program = m_program;
        MAYBE_UNUSED auto program_binder = program.bind();
        program.setUniforms(mvp, renderState.uniforms);
        virt_setCustomUniforms(gl, renderState);

        MAYBE_UNUSED auto renderStateBinder = RenderStateBinder{gl, gl.getTexLookup(), renderState};
        trySetupVaoAttribs();

        MAYBE_UNUSED auto vao_binder = m_vao.bind();
        virt_draw(gl);
    }

protected:
    NODISCARD DrawModeEnum getDrawMode() const { return m_drawMode; }
    NODISCARD GLsizei getNumVerts() const { return m_numVerts; }
};

template<typename VertexType_, typename ProgramType_>
void SimpleMeshBase<VertexType_, ProgramType_>::virt_setupAttribs()
{
    MMLOG_ERROR() << "Fatal error: called pure virtual function.";
    std::abort();
}

template<typename VertexType_, typename ProgramType_>
void SimpleMeshBase<VertexType_, ProgramType_>::virt_setCustomUniforms(
    Functions & /*gl*/, const GLRenderState & /*renderState*/)
{
    MMLOG_ERROR() << "Fatal error: called pure virtual function.";
    std::abort();
}

template<typename VertexType_, typename ProgramType_>
void SimpleMeshBase<VertexType_, ProgramType_>::virt_draw(Functions & /*gl*/)
{
    MMLOG_ERROR() << "Fatal error: called pure virtual function.";
    std::abort();
}

template<typename VertexType_, typename ProgramType_>
class NODISCARD SimpleMesh : public SimpleMeshBase<VertexType_, ProgramType_>
{
    using Base = SimpleMeshBase<VertexType_, ProgramType_>;

public:
    using Base::Base;

private:
    void virt_setCustomUniforms(Functions & /*gl*/, const GLRenderState & /*renderState*/) final {}

private:
    void virt_draw(Functions &gl) final
    {
        const DrawModeEnum mode = Base::getDrawMode();
        const auto numVerts = Base::getNumVerts();
        if (mode == DrawModeEnum::INSTANCED_QUADS) {
            drawRoomQuad(gl, numVerts);
        } else if (const std::optional<GLenum> &optMode = gl.toGLenum(mode)) {
            gl.glDrawArrays(optMode.value(), 0, numVerts);
        } else {
            assert(false);
        }
    }
};

} // namespace Legacy
