// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "FontMesh3d.h"

#include "../../display/Textures.h"

#include <memory>
#include <vector>

namespace Legacy {

FontMesh3d::FontMesh3d(const SharedFunctions &functions,
                       const std::shared_ptr<FontShader> &sharedShader,
                       SharedMMTexture texture,
                       const DrawModeEnum mode,
                       const std::vector<FontVert3d> &verts)
    : Base{functions, sharedShader, mode, verts}
    , m_texture{std::move(texture)}
{}

FontMesh3d::~FontMesh3d() = default;

GLRenderState FontMesh3d::virt_modifyRenderState(const GLRenderState &renderState) const
{
    const SharedMMTexture &shared = m_texture;
    const MMTexture &tex = *shared;
    const MMTextureId id = tex.getId();

    return renderState.withBlend(BlendModeEnum::TRANSPARENCY)
        .withDepthFunction(std::nullopt)
        .withTexture0(id);
}

} // namespace Legacy
