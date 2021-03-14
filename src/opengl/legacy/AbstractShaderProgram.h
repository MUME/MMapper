#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Legacy.h"

namespace Legacy {

static constexpr GLuint INVALID_ATTRIB_LOCATION = ~0u;
static constexpr GLint INVALID_UNIFORM_LOCATION = -1;

struct NODISCARD AbstractShaderProgram
{
protected:
    std::string m_dirName;
    WeakFunctions m_functions;
    GLuint m_program = 0;
    bool m_isBound = false;

public:
    AbstractShaderProgram() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(AbstractShaderProgram);

public:
    AbstractShaderProgram(std::string dirName, const SharedFunctions &functions, GLuint program);

    virtual ~AbstractShaderProgram();

public:
    class NODISCARD ProgramUnbinder final
    {
    private:
        AbstractShaderProgram *self = nullptr;

    public:
        ProgramUnbinder() = delete;
        DEFAULT_MOVES_DELETE_COPIES(ProgramUnbinder);

    public:
        explicit ProgramUnbinder(AbstractShaderProgram &_self)
            : self{&_self}
        {}
        ~ProgramUnbinder()
        {
            if (self)
                self->unbind();
        }
    };

    NODISCARD ProgramUnbinder bind();

private:
    friend ProgramUnbinder;
    // REVISIT: store the currently bound program on the functions object?
    void unbind();

public:
    // set program uniforms...
    void setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms);

protected:
    virtual void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) = 0;

public:
    NODISCARD GLuint getAttribLocation(const char *name) const;
    NODISCARD GLint getUniformLocation(const char *name) const;
    NODISCARD bool hasUniform(const char *name) const;

public:
    void setUniform1iv(GLint location, GLsizei count, const GLint *value);
    void setUniform1fv(GLint location, GLsizei count, const GLfloat *value);
    void setUniform4fv(GLint location, GLsizei count, const GLfloat *value);
    void setUniform4iv(GLint location, GLsizei count, const GLint *value);
    void setUniformMatrix4fv(GLint location,
                             GLsizei count,
                             GLboolean transpose,
                             const GLfloat *value);

private:
    NODISCARD float getDevicePixelRatio() const;

public:
    void setPointSize(float in_pointSize);
    void setColor(const char *name, const Color &color);
    void setMatrix(const char *name, const glm::mat4 &m);
    void setTexture(const char *name, int textureUnit);
    void setViewport(const char *name, const Viewport &input_viewport);
};

} // namespace Legacy
