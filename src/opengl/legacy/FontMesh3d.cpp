// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "FontMesh3d.h"

namespace Legacy {

FontMesh3d::FontMesh3d(const SharedFunctions &functions,
                       const std::shared_ptr<FontShader> &sharedShader,
                       SharedMMTexture texture,
                       const DrawModeEnum mode,
                       const std::vector<FontVert3d> &verts)
    : Base(functions, sharedShader, mode, verts)
    , m_texture{std::move(texture)}
{}

FontMesh3d::~FontMesh3d() = default;

void FontMesh3d::render(const GLRenderState &renderState)
{
    const auto state = renderState.withBlend(BlendModeEnum::TRANSPARENCY)
                           .withDepthFunction(std::nullopt)
                           .withTexture0(m_texture);
    Base::render(state);
}

} // namespace Legacy
