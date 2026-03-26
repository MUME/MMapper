// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TFO.h"

#include "Legacy.h"

namespace Legacy {

void TFO::emplace(const SharedFunctions &sharedFunctions)
{
    reset();
    m_weakFunctions = sharedFunctions;
    sharedFunctions->glGenTransformFeedbacks(1, &m_tfo);
}

void TFO::reset()
{
    if (m_tfo != INVALID_TFO) {
        if (auto shared = m_weakFunctions.lock()) {
            shared->glDeleteTransformFeedbacks(1, &m_tfo);
        }
        m_tfo = INVALID_TFO;
    }
}

GLuint TFO::get() const
{
    return m_tfo;
}

} // namespace Legacy
