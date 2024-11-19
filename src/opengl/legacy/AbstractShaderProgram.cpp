// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "AbstractShaderProgram.h"

namespace Legacy {

AbstractShaderProgram::AbstractShaderProgram(std::string dirName,
                                             SharedFunctions functions,
                                             Program program)
    : m_dirName{std::move(dirName)}
    , m_functions{std::move(functions)}
    , m_program{std::move(program)}
{}

AbstractShaderProgram::~AbstractShaderProgram()
{
    assert(!m_isBound);
}

AbstractShaderProgram::ProgramUnbinder AbstractShaderProgram::bind()
{
    assert(!m_isBound);
    if (auto f = m_functions.lock()) {
        f->glUseProgram(getProgram());
    }
    m_isBound = true;
    return ProgramUnbinder{*this};
}

void AbstractShaderProgram::unbind()
{
    assert(m_isBound);
    if (auto f = m_functions.lock()) {
        f->glUseProgram(0);
    }
    m_isBound = false;
}

void AbstractShaderProgram::setUniforms(const glm::mat4 &mvp,
                                        const GLRenderState::Uniforms &uniforms)
{
    assert(m_isBound);
    virt_setUniforms(mvp, uniforms);

    if (uniforms.pointSize) {
        setPointSize(uniforms.pointSize.value());
    }
}

GLuint AbstractShaderProgram::getAttribLocation(const char *const name) const
{
    assert(name != nullptr);
    assert(m_isBound);
    auto functions = m_functions.lock();
    const auto tmp = deref(functions).glGetAttribLocation(getProgram(), name);
    // Reason for making the cast here: glGetAttribLocation uses signed GLint,
    // but glVertexAttribXXX() uses unsigned GLuint.
    const auto result = static_cast<GLuint>(tmp);
    assert(result != INVALID_ATTRIB_LOCATION);
    return result;
}

GLint AbstractShaderProgram::getUniformLocation(const char *const name) const
{
    assert(name != nullptr);
    assert(m_isBound);
    auto functions = m_functions.lock();
    const auto result = deref(functions).glGetUniformLocation(getProgram(), name);
    assert(result != INVALID_UNIFORM_LOCATION);
    return result;
}

bool AbstractShaderProgram::hasUniform(const char *const name) const
{
    assert(name != nullptr);
    auto functions = m_functions.lock();
    const auto result = deref(functions).glGetUniformLocation(getProgram(), name);
    return result != INVALID_UNIFORM_LOCATION;
}

void AbstractShaderProgram::setUniform1iv(const GLint location,
                                          const GLsizei count,
                                          const GLint *const value)
{
    assert(m_isBound);
    auto functions = m_functions.lock();
    deref(functions).glUniform1iv(location, count, value);
}

void AbstractShaderProgram::setUniform1fv(const GLint location,
                                          const GLsizei count,
                                          const GLfloat *const value)
{
    assert(m_isBound);
    auto functions = m_functions.lock();
    deref(functions).glUniform1fv(location, count, value);
}

void AbstractShaderProgram::setUniform4fv(const GLint location,
                                          const GLsizei count,
                                          const GLfloat *const value)
{
    assert(m_isBound);
    auto functions = m_functions.lock();
    deref(functions).glUniform4fv(location, count, value);
}

void AbstractShaderProgram::setUniform4iv(const GLint location,
                                          const GLsizei count,
                                          const GLint *const value)
{
    assert(m_isBound);
    auto functions = m_functions.lock();
    deref(functions).glUniform4iv(location, count, value);
}

void AbstractShaderProgram::setUniformMatrix4fv(const GLint location,
                                                const GLsizei count,
                                                const GLboolean transpose,
                                                const GLfloat *const value)
{
    assert(m_isBound);
    auto functions = m_functions.lock();
    deref(functions).glUniformMatrix4fv(location, count, transpose, value);
}

float AbstractShaderProgram::getDevicePixelRatio() const
{
    auto functions = m_functions.lock();
    return deref(functions).getDevicePixelRatio();
}

void AbstractShaderProgram::setPointSize(const float in_pointSize)
{
    const auto location = getUniformLocation("uPointSize");
    if (location != INVALID_UNIFORM_LOCATION) {
        const float pointSize = in_pointSize * getDevicePixelRatio();
        setUniform1fv(location, 1, &pointSize);
    }
}

void AbstractShaderProgram::setColor(const char *const name, const Color &color)
{
    const auto location = getUniformLocation(name);
    const glm::vec4 v = color.getVec4();
    setUniform4fv(location, 1, glm::value_ptr(v));
}

void AbstractShaderProgram::setMatrix(const char *const name, const glm::mat4 &m)
{
    const auto location = getUniformLocation(name);
    setUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

void AbstractShaderProgram::setTexture(const char *const name, const int textureUnit)
{
    assert(textureUnit >= 0);
    const GLint uFontTextureLoc = getUniformLocation(name);
    setUniform1iv(uFontTextureLoc, 1, &textureUnit);
}

void AbstractShaderProgram::setViewport(const char *const name, const Viewport &input_viewport)
{
    const glm::ivec4 viewport{input_viewport.offset, input_viewport.size};
    const GLint location = getUniformLocation(name);
    setUniform4iv(location, 1, glm::value_ptr(viewport));
}

} // namespace Legacy
