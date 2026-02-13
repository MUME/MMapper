#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "../../global/RuleOf5.h"
#include "../OpenGLTypes.h"

#include <memory>

#include <QOpenGLFramebufferObject>
#include <QSize>

namespace Legacy {

extern bool LOG_FBO_ALLOCATIONS;

class FBO final
{
public:
    FBO() = default;
    ~FBO() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(FBO);

public:
    void configure(const Viewport &physicalViewport, int samples);
    void bind();
    void release();
    void resolve();

    NODISCARD GLuint resolvedTextureId() const;

private:
    std::unique_ptr<QOpenGLFramebufferObject> m_multisamplingFbo;
    std::unique_ptr<QOpenGLFramebufferObject> m_resolvedFbo;
};

} // namespace Legacy
