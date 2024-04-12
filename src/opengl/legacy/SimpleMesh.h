#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cassert>

#include "../OpenGLTypes.h"
#include "AbstractShaderProgram.h"
#include "Binders.h"
#include "Legacy.h"
#include "VBO.h"

namespace Legacy {

template<typename VertexType_, typename ProgramType_>
class NODISCARD SimpleMesh : public IRenderable
{
public:
    using ProgramType = ProgramType_;
    static_assert(std::is_base_of_v<AbstractShaderProgram, ProgramType>);

protected:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ProgramType_> m_shared_program;
    ProgramType_ &m_program;
    VBO m_vbo;
    DrawModeEnum m_drawMode = DrawModeEnum::INVALID;
    GLsizei m_numVerts = 0;

public:
    explicit SimpleMesh(SharedFunctions sharedFunctions, std::shared_ptr<ProgramType_> sharedProgram)
        : m_shared_functions{std::move(sharedFunctions)}
        , m_functions{deref(m_shared_functions)}
        , m_shared_program{std::move(sharedProgram)}
        , m_program{deref(m_shared_program)}
    {}

    explicit SimpleMesh(const SharedFunctions &sharedFunctions,
                        const std::shared_ptr<ProgramType_> &sharedProgram,
                        const DrawModeEnum mode,
                        const std::vector<VertexType_> &verts)
        : SimpleMesh{sharedFunctions, sharedProgram}
    {
        setStatic(mode, verts);
    }

    ~SimpleMesh() override { reset(); }

protected:
    class NODISCARD AttribUnbinder final
    {
    private:
        SimpleMesh *self = nullptr;

    public:
        AttribUnbinder() = delete;
        DEFAULT_MOVES_DELETE_COPIES(AttribUnbinder);

    public:
        explicit AttribUnbinder(SimpleMesh &_self)
            : self{&_self}
        {}
        ~AttribUnbinder()
        {
            if (self != nullptr)
                self->unbindAttribs();
        }
    };

    AttribUnbinder bindAttribs()
    {
        virt_bind();
        return AttribUnbinder{*this};
    }

private:
    friend AttribUnbinder;
    void unbindAttribs() { virt_unbind(); }
    virtual void virt_bind() = 0;
    virtual void virt_unbind() = 0;

public:
    void unsafe_swapVboId(VBO &vbo) { return m_vbo.unsafe_swapVboId(vbo); }

public:
    void setDynamic(const DrawModeEnum mode, const std::vector<VertexType_> &verts)
    {
        setCommon(mode, verts, BufferUsageEnum::DYNAMIC_DRAW);
    }
    void setStatic(const DrawModeEnum mode, const std::vector<VertexType_> &verts)
    {
        setCommon(mode, verts, BufferUsageEnum::STATIC_DRAW);
    }

private:
    void setCommon(const DrawModeEnum mode,
                   const std::vector<VertexType_> &verts,
                   const BufferUsageEnum usage)
    {
        const auto numVerts = verts.size();
        assert(mode == DrawModeEnum::INVALID || numVerts % static_cast<size_t>(mode) == 0);

        if (!m_vbo && numVerts != 0) {
            m_vbo.emplace(m_shared_functions);
        }

        if (LOG_VBO_STATIC_UPLOADS && usage == BufferUsageEnum::STATIC_DRAW && m_vbo) {
            qInfo() << "Uploading static buffer with" << numVerts << "verts of size"
                    << sizeof(VertexType_) << "(total" << (numVerts * sizeof(VertexType_))
                    << "bytes) to VBO" << m_vbo.get() << __FUNCTION__;
        }

        if (m_vbo) {
            auto tmp = m_functions.setVbo(mode, m_vbo.get(), verts, usage);
            m_drawMode = tmp.first;
            m_numVerts = tmp.second;
        } else {
            // REVISIT: Should this be reported as an error?
            m_drawMode = DrawModeEnum::INVALID;
            m_numVerts = 0;
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
        m_drawMode = DrawModeEnum::INVALID;
        m_numVerts = 0;
        m_vbo.reset();
        assert(isEmpty() && !m_vbo);
    }

private:
    NODISCARD bool virt_isEmpty() const final
    {
        return !m_vbo || m_numVerts == 0 || m_drawMode == DrawModeEnum::INVALID;
    }

private:
    void virt_render(const GLRenderState &renderState) final
    {
        if (isEmpty())
            return;

        m_functions.checkError();

        const glm::mat4 mvp = m_functions.getProjectionMatrix();
        auto programUnbinder = m_program.bind();
        m_program.setUniforms(mvp, renderState.uniforms);
        RenderStateBinder renderStateBinder(m_functions, renderState);
        auto attribUnbinder = bindAttribs(); // mesh sets its own attributes

        m_functions.checkError();

        if (const std::optional<GLenum> &optMode = Functions::toGLenum(m_drawMode)) {
            m_functions.glDrawArrays(optMode.value(), 0, m_numVerts);
        } else {
            assert(false);
        }

        m_functions.checkError();
    }
};
} // namespace Legacy
