// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Binders.h"

#include "../../display/Textures.h" // modularity violation

#include <optional>

namespace Legacy {

BlendBinder::BlendBinder(Functions &in_functions, const BlendModeEnum in_blend)
    : functions{in_functions}
    , blend{in_blend}
{
    switch (blend) {
    case BlendModeEnum::NONE:
        functions.glDisable(GL_BLEND);
        break;
    case BlendModeEnum::TRANSPARENCY:
        functions.glEnable(GL_BLEND);
        functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BlendModeEnum::MODULATE:
        functions.glEnable(GL_BLEND);
        functions.glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
        break;
    }
}

BlendBinder::~BlendBinder()
{
    switch (blend) {
    case BlendModeEnum::NONE:
    case BlendModeEnum::TRANSPARENCY:
        break;
    case BlendModeEnum::MODULATE:
        functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
    functions.glDisable(GL_BLEND);
}

CullingBinder::CullingBinder(Functions &in_functions, const CullingEnum &in_culling)
    : functions{in_functions}
{
    switch (in_culling) {
    case CullingEnum::DISABLED:
        disable();
        break;
    case CullingEnum::BACK:
        enable(GL_BACK);
        break;
    case CullingEnum::FRONT:
        enable(GL_FRONT);
        break;
    case CullingEnum::FRONT_AND_BACK:
        enable(GL_FRONT_AND_BACK);
        break;
    }
}

CullingBinder::~CullingBinder()
{
    disable();
}

void CullingBinder::enable(const GLenum mode)
{
    functions.glCullFace(mode);
    functions.glEnable(GL_CULL_FACE);
}

void CullingBinder::disable()
{
    functions.glDisable(GL_CULL_FACE);
    functions.glCullFace(GL_BACK);
}

DepthBinder::DepthBinder(Functions &in_functions, const DepthBinder::OptDepth &in_depth)
    : functions{in_functions}
    , depth{in_depth}
{
    if (depth) {
        functions.glEnable(GL_DEPTH_TEST);
        functions.glDepthFunc(static_cast<GLenum>(depth.value()));
    } else {
        functions.glDisable(GL_DEPTH_TEST);
    }
}

DepthBinder::~DepthBinder()
{
    if (depth) {
        functions.glDisable(GL_DEPTH_TEST);
        functions.glDepthFunc(GL_LESS);
    }
}

LineParamsBinder::LineParamsBinder(Functions &in_functions, const LineParams &in_lineParams)
    : functions{in_functions}
    , lineParams{in_lineParams}
{
    functions.glLineWidth(lineParams.width);
}

LineParamsBinder::~LineParamsBinder()
{
    const auto &width = lineParams.width;
    const float ONE = 1.f;
    static_assert(sizeof(ONE) == sizeof(width));
    if (!utils::equals(&width, &ONE)) {
        functions.glLineWidth(ONE);
    }
}

PointSizeBinder::PointSizeBinder(Functions &in_functions, const std::optional<GLfloat> &in_pointSize)
    : functions{in_functions}
    , optPointSize{in_pointSize}
{
    if (optPointSize.has_value()) {
        functions.enableProgramPointSize(true);
    }
}

PointSizeBinder::~PointSizeBinder()
{
    if (optPointSize.has_value()) {
        functions.enableProgramPointSize(false);
    }
}

TexturesBinder::TexturesBinder(const TexLookup &in_lookup,
                               const TexturesBinder::Textures &in_textures)
    : lookup{in_lookup}
    , textures{in_textures}
{
    for (size_t i = 0, size = textures.size(); i < size; ++i) {
        const auto id = textures[i];
        if (id == INVALID_MM_TEXTURE_ID) {
            continue;
        }

        // note: pointer to shared pointer
        if (const SharedMMTexture *const pShared = lookup.find(id)) {
            if (const SharedMMTexture &tex = *pShared) {
                tex->bind(static_cast<uint32_t>(i));
            }
        }
    }
}

TexturesBinder::~TexturesBinder()
{
    for (size_t i = 0, size = textures.size(); i < size; ++i) {
        const auto id = textures[i];
        if (id == INVALID_MM_TEXTURE_ID) {
            continue;
        }

        // note: pointer to shared pointer
        if (const SharedMMTexture *const pShared = lookup.find(id)) {
            if (const SharedMMTexture &tex = *pShared) {
                tex->release(static_cast<uint32_t>(i));
            }
        }
    }
}

RenderStateBinder::RenderStateBinder(Functions &functions,
                                     const TexLookup &texLookup,
                                     const GLRenderState &renderState)
    : blendBinder{functions, renderState.blend}
    , cullingBinder{functions, renderState.culling}
    , depthBinder{functions, renderState.depth}
    , lineParamsBinder{functions, renderState.lineParams}
    , pointSizeBinder{functions, renderState.uniforms.pointSize}
    , texturesBinder{texLookup, renderState.uniforms.textures}
{}

} // namespace Legacy
