#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/RAII.h"
#include "../OpenGLTypes.h"
#include "AbstractShaderProgram.h"
#include "Binders.h"
#include "Legacy.h"
#include "VAO.h"
#include "VBO.h"

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

namespace Legacy {

void drawRoomQuad(Functions &gl, GLsizei numVerts);

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
    VAO m_vao;
    DrawModeEnum m_drawMode = DrawModeEnum::INVALID;
    GLsizei m_numVerts = 0;

public:
    explicit SimpleMesh(SharedFunctions sharedFunctions, std::shared_ptr<ProgramType_> sharedProgram)
        : m_shared_functions{std::move(sharedFunctions)}
        , m_functions{deref(m_shared_functions)}
        , m_shared_program{std::move(sharedProgram)}
        , m_program{deref(m_shared_program)}
    {
        m_vao.emplace(m_shared_functions);
    }

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
        SimpleMesh *m_self = nullptr;

    public:
        AttribUnbinder() = delete;
        DEFAULT_MOVES_DELETE_COPIES(AttribUnbinder);

    public:
        explicit AttribUnbinder(SimpleMesh &self)
            : m_self{&self}
        {}
        ~AttribUnbinder()
        {
            if (m_self != nullptr) {
                m_self->unbindAttribs();
            }
        }
    };

    NODISCARD AttribUnbinder bindAttribs()
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

        static_assert(static_cast<size_t>(DrawModeEnum::POINTS) == 1);
        static_assert(static_cast<size_t>(DrawModeEnum::LINES) == 2);
        static_assert(static_cast<size_t>(DrawModeEnum::TRIANGLES) == 3);
        static_assert(static_cast<size_t>(DrawModeEnum::QUADS) == 4);
        assert(mode == DrawModeEnum::INVALID || mode == DrawModeEnum::INSTANCED_QUADS
               || numVerts % static_cast<size_t>(mode) == 0);

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
        m_vao.reset();
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
        if (isEmpty()) {
            return;
        }

        m_functions.checkError();

        const glm::mat4 mvp = m_functions.getProjectionMatrix();
        auto programUnbinder = m_program.bind();
        m_program.setUniforms(mvp, renderState.uniforms);
        RenderStateBinder renderStateBinder(m_functions, m_functions.getTexLookup(), renderState);

        m_functions.glBindVertexArray(m_vao.get());
        RAIICallback vaoUnbinder([&]() {
            m_functions.glBindVertexArray(0);
            m_functions.checkError();
        });

        auto attribUnbinder = bindAttribs();

        if (m_drawMode == DrawModeEnum::INSTANCED_QUADS) {
            drawRoomQuad(m_functions, m_numVerts);
        } else if (const std::optional<GLenum> &optMode = m_functions.toGLenum(m_drawMode)) {
            m_functions.glDrawArrays(optMode.value(), 0, m_numVerts);
        } else {
            assert(false);
        }
    }
};

} // namespace Legacy
