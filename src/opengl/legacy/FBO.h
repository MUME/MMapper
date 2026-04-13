#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "../../global/RuleOf5.h"
#include "../OpenGLTypes.h"

#include <memory>

#include <QOpenGLFramebufferObject>
#include <QSize>

namespace Legacy {

class NODISCARD FBO final
{
private:
    std::unique_ptr<QOpenGLFramebufferObject> m_multisamplingFbo;
    std::unique_ptr<QOpenGLFramebufferObject> m_defaultFbo;
    enum class NODISCARD BindStateEnum : uint8_t { Unbound, DefaultIsBound, MultisampleIsBound };
    BindStateEnum m_bindState = BindStateEnum::Unbound;

public:
    explicit FBO() = default;
    ~FBO();
    DELETE_CTORS_AND_ASSIGN_OPS(FBO);

public:
    void configure(const Viewport &physicalViewport, int samples);
    void bind();
    void release();
    void resolve();

    NODISCARD GLuint resolvedTextureId() const;

private:
    NODISCARD QOpenGLFramebufferObject *getDrawable();
};

} // namespace Legacy
