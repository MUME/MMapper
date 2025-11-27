// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "VAO.h"

#include <QDebug>
#include <QOpenGLContext>

namespace Legacy {

bool LOG_VAO_ALLOCATIONS = false;

void VAO::emplace(const SharedFunctions &sharedFunctions)
{
    assert(!*this);
    m_weakFunctions = sharedFunctions;
    const auto shared = m_weakFunctions.lock();
    if (shared == nullptr) {
        throw std::runtime_error("Legacy::Functions is no longer valid");
    }

    shared->glGenVertexArrays(1, &m_vao);
    shared->checkError();

    if (LOG_VAO_ALLOCATIONS) {
        qInfo() << "Allocated VAO" << m_vao;
    }
}

void VAO::reset()
{
    if (!*this) {
        return;
    }

    const auto shared = m_weakFunctions.lock();
    if (shared != nullptr) {
        if (LOG_VAO_ALLOCATIONS) {
            qInfo() << "Deallocating VAO" << m_vao;
        }
        shared->glDeleteVertexArrays(1, &m_vao);
        shared->checkError();
    } else {
        qCritical() << "Legacy::Functions is no longer valid, leaking VAO" << m_vao;
    }

    m_vao = INVALID_VAOID;
    m_weakFunctions.reset();
}

GLuint VAO::get() const
{
    return m_vao;
}

} // namespace Legacy
