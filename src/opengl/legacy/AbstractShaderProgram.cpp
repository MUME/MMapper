// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "AbstractShaderProgram.h"

#include "../../global/ConfigConsts.h"

#include <cassert>

namespace Legacy {

AbstractShaderProgram::AbstractShaderProgram(std::string dirName,
                                             const SharedFunctions &functions,
                                             Program program)
    : m_dirName{std::move(dirName)}
    , m_functions{functions} // conversion to weak ptr
    , m_program{std::move(program)}
{
    if (functions == nullptr) {
        MMLOG_WARNING() << "AbstractShaderProgram constructed with null functions";
    }
}

AbstractShaderProgram::~AbstractShaderProgram()
{
    assert(!m_isBound);
}

AbstractShaderProgram::ProgramUnbinder AbstractShaderProgram::bind()
{
    assert(!m_isBound);
    if (const auto shared_funcs = m_functions.lock(); shared_funcs == nullptr) {
        MMLOG_WARNING() << "Failed to use program " << getProgram();
    } else {
        shared_funcs->glUseProgram(getProgram());
    }
    m_isBound = true;
    return ProgramUnbinder{*this};
}

void AbstractShaderProgram::unbind()
{
    assert(m_isBound);
    m_isBound = false;
    if (const auto shared_funcs = m_functions.lock(); shared_funcs == nullptr) {
        MMLOG_WARNING() << "Failed to stop using program " << getProgram();
    } else {
        shared_funcs->glUseProgram(0);
    }
}

void AbstractShaderProgram::setUniforms(const glm::mat4 &mvp,
                                        const GLRenderState::Uniforms &uniforms)
{
    assert(m_isBound);
    virt_setUniforms(mvp, uniforms);

    if (uniforms.pointSize.has_value()) {
        setPointSize(uniforms.pointSize.value());
    }
}

GLuint AbstractShaderProgram::getAttribLocation(const char *const name) const
{
    if (name == nullptr) {
        throw std::invalid_argument("name");
    }
    assert(m_isBound);

    if (const auto shared_funcs = m_functions.lock(); shared_funcs == nullptr) {
        // REVISIT: should this throw?
        assert(false);
        return INVALID_ATTRIB_LOCATION;
    } else {
        const auto tmp = deref(shared_funcs).glGetAttribLocation(getProgram(), name);
        // Reason for making the cast here: glGetAttribLocation uses signed GLint,
        // but glVertexAttribXXX() uses unsigned GLuint.
        const auto result = static_cast<GLuint>(tmp);
        assert(result != INVALID_ATTRIB_LOCATION);
        return result;
    }
}

GLint AbstractShaderProgram::getUniformLocation(const char *const name) const
{
    if (name == nullptr) {
        throw std::invalid_argument("name");
    }
    assert(m_isBound);
    if (const auto shared_funcs = m_functions.lock(); shared_funcs == nullptr) {
        // REVISIT: should this throw?
        assert(false);
        return INVALID_UNIFORM_LOCATION;
    } else {
        const auto result = deref(shared_funcs).glGetUniformLocation(getProgram(), name);
        assert(result != INVALID_UNIFORM_LOCATION);
        return result;
    }
}

bool AbstractShaderProgram::hasUniform(const char *const name) const
{
    assert(name != nullptr);
    const auto shared_funcs = m_functions.lock();
    const auto result = deref(shared_funcs).glGetUniformLocation(getProgram(), name);
    return result != INVALID_UNIFORM_LOCATION;
}

void AbstractShaderProgram::setUniform1iv(const GLint location,
                                          const GLsizei count,
                                          const GLint *const value)
{
    assert(m_isBound);
    const auto shared_funcs = m_functions.lock();
    deref(shared_funcs).glUniform1iv(location, count, value);
}

void AbstractShaderProgram::setUniform1fv(const GLint location,
                                          const GLsizei count,
                                          const GLfloat *const value)
{
    assert(m_isBound);
    const auto shared_funcs = m_functions.lock();
    deref(shared_funcs).glUniform1fv(location, count, value);
}

void AbstractShaderProgram::setUniform4fv(const GLint location,
                                          const GLsizei count,
                                          const GLfloat *const value)
{
    assert(m_isBound);
    const auto shared_funcs = m_functions.lock();
    deref(shared_funcs).glUniform4fv(location, count, value);
}

void AbstractShaderProgram::setUniform4iv(const GLint location,
                                          const GLsizei count,
                                          const GLint *const value)
{
    assert(m_isBound);
    const auto shared_funcs = m_functions.lock();
    deref(shared_funcs).glUniform4iv(location, count, value);
}

void AbstractShaderProgram::setUniformMatrix4fv(const GLint location,
                                                const GLsizei count,
                                                const GLboolean transpose,
                                                const GLfloat *const value)
{
    assert(m_isBound);
    const auto shared_funcs = m_functions.lock();
    deref(shared_funcs).glUniformMatrix4fv(location, count, transpose, value);
}

float AbstractShaderProgram::getDevicePixelRatio() const
{
    const auto shared_funcs = m_functions.lock();
    return deref(shared_funcs).getDevicePixelRatio();
}

void AbstractShaderProgram::setPointSize(const float in_pointSize)
{
    if (const auto location = getUniformLocation("uPointSize");
        location == INVALID_UNIFORM_LOCATION) {
        MMLOG_WARNING() << "Failed to set point size due to invalid location for uPointSize";
    } else {
        const float pointSize = in_pointSize * getDevicePixelRatio();
        setUniform1fv(location, 1, &pointSize);
    }
}

void AbstractShaderProgram::setColor(const char *const name, const Color color)
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

void AbstractShaderProgram::setFloat(const char *const name, const float value)
{
    const GLint location = getUniformLocation(name);
    setUniform1fv(location, 1, &value);
}

} // namespace Legacy
