// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "VBO.h"

namespace Legacy {
bool LOG_VBO_ALLOCATIONS = false;
bool LOG_VBO_STATIC_UPLOADS = false;

void VBO::emplace(const SharedFunctions &sharedFunctions)
{
    if (m_vbo == INVALID_VBOID) {
        m_weakFunctions = sharedFunctions;
        deref(sharedFunctions).glGenBuffers(1, &m_vbo);
        if (LOG_VBO_ALLOCATIONS) {
            qInfo() << this << "Allocated VBO" << m_vbo;
        }
        assert(m_vbo != INVALID_VBOID);
    }
}

void VBO::reset()
{
    if (auto vbo = std::exchange(m_vbo, INVALID_VBOID); vbo != INVALID_VBOID) {
        if (LOG_VBO_ALLOCATIONS) {
            qInfo() << this << "Freeing VBO" << vbo;
        }
        auto sharedFunctions = std::exchange(m_weakFunctions, {}).lock();
        deref(sharedFunctions).glDeleteBuffers(1, &vbo);
    }
    assert(m_weakFunctions.lock() == nullptr);
}

GLuint VBO::get() const
{
    if (m_vbo == INVALID_VBOID)
        throw std::runtime_error("VBO not allocated");
    return m_vbo;
}

void Program::swapWith(Program &other) noexcept
{
    assert(&other != this);
    std::swap(m_weakFunctions, other.m_weakFunctions);
    std::swap(m_program, other.m_program);
}

Program::Program(Program &&other) noexcept
{
    swapWith(other);
}

Program &Program::operator=(Program &&other) noexcept
{
    if (&other != this) {
        swapWith(other);
    }
    return *this;
}

void Program::emplace(const SharedFunctions &sharedFunctions)
{
    if (m_program == INVALID_PROGRAM) {
        m_weakFunctions = sharedFunctions;
        m_program = deref(sharedFunctions).glCreateProgram();
        if (LOG_VBO_ALLOCATIONS) {
            qInfo() << this << "Allocated Shader Program" << m_program;
        }
        assert(m_program != INVALID_PROGRAM);
    }
}

void Program::reset()
{
    if (auto program = std::exchange(m_program, INVALID_PROGRAM); program != INVALID_PROGRAM) {
        if (LOG_VBO_ALLOCATIONS) {
            qInfo() << this << "Freeing Shader Program" << program;
        }
        auto sharedFunctions = std::exchange(m_weakFunctions, {}).lock();
        deref(sharedFunctions).glDeleteProgram(program);
    }
    assert(m_weakFunctions.lock() == nullptr);
}

GLuint Program::get() const
{
    if (m_program == INVALID_PROGRAM)
        throw std::runtime_error("Shader Program not allocated");
    return m_program;
}

} // namespace Legacy
