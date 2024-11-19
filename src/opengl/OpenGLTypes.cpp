// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "OpenGLTypes.h"

#include "../display/Textures.h"
#include "../global/utils.h"

#include <algorithm>
#include <memory>

IRenderable::~IRenderable() = default;

TexturedRenderable::TexturedRenderable(const SharedMMTexture &tex,
                                       std::unique_ptr<IRenderable> moved_mesh)
    : m_texture(tex)
    , m_mesh(std::move(moved_mesh))
{
    std::ignore = deref(m_mesh);
}

TexturedRenderable::~TexturedRenderable() = default;

void TexturedRenderable::virt_clear()
{
    m_mesh->clear();
}

void TexturedRenderable::virt_reset()
{
    m_texture.reset();
    m_mesh->reset();
}

bool TexturedRenderable::virt_isEmpty() const
{
    return m_mesh->isEmpty();
}

void TexturedRenderable::virt_render(const GLRenderState &renderState)
{
    /* overrides the texture of the provided state */
    m_mesh->render(renderState.withTexture0(m_texture));
}
