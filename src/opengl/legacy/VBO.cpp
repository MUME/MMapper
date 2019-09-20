// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "VBO.h"

namespace Legacy {
bool LOG_VBO_ALLOCATIONS = false;
bool LOG_VBO_STATIC_UPLOADS = false;

void VBO::emplace(const SharedFunctions &sharedFunctions)
{
    if (!m_vbo) {
        m_weakFunctions = sharedFunctions;
        deref(sharedFunctions).glGenBuffers(1, &m_vbo);
        if (LOG_VBO_ALLOCATIONS) {
            qInfo() << this << "Allocated VBO" << m_vbo;
        }
    }
}

void VBO::reset()
{
    if (auto vbo = std::exchange(m_vbo, 0)) {
        if (LOG_VBO_ALLOCATIONS) {
            qInfo() << this << "Freeing VBO" << vbo;
        }
        auto sharedFunctions = std::exchange(m_weakFunctions, {}).lock();
        deref(sharedFunctions).glDeleteBuffers(1, &vbo);
    }
    assert(m_weakFunctions.lock() == nullptr);
}

GLuint VBO::get()
{
    if (m_vbo == 0)
        throw std::runtime_error("VBO not allocated");
    return m_vbo;
}

} // namespace Legacy
