// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "SimpleMesh.h"

#include "../../global/ConfigConsts.h"

void Legacy::drawRoomQuad(Functions &gl, const GLsizei numVerts)
{
    static constexpr size_t NUM_ELEMENTS = 4;
    const SharedVbo shared = gl.getSharedVbos().get(SharedVboEnum::InstancedQuadIbo);
    VBO &vbo = deref(shared);

    if (!vbo) {
        if (IS_DEBUG_BUILD) {
            qDebug() << "allocating shared VBO for drawRoomQuad";
        }

        vbo.emplace(gl.shared_from_this());

        // CCW order
        const std::vector<uint8_t> indices{0, 1, 2, 3};
        assert(indices.size() == NUM_ELEMENTS);

        MAYBE_UNUSED const auto numIndices = gl.setIbo(vbo.get(),
                                                       indices,
                                                       BufferUsageEnum::STATIC_DRAW);
        assert(numIndices == NUM_ELEMENTS);
    }

    struct NODISCARD IBOBinder final
    {
    private:
        Functions &m_gl;

    public:
        IBOBinder(Functions &f, const VBO &vbo)
            : m_gl(f)
        {
            if (IS_DEBUG_BUILD) {
                GLint binding = -1;
                m_gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &binding);
                assert(binding == 0);
            }
            m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.get());
        }
        ~IBOBinder() { m_gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
        DELETE_CTORS_AND_ASSIGN_OPS(IBOBinder);
    };

    {
        IBOBinder ibo_binder{gl, *shared};
        gl.glDrawElementsInstanced(GL_TRIANGLE_FAN,
                                   NUM_ELEMENTS,
                                   GL_UNSIGNED_BYTE,
                                   nullptr,
                                   numVerts);
    }
}
