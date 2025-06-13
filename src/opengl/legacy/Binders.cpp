// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Binders.h"

#include "../../display/Textures.h" // modularity violation

#include <optional>

namespace Legacy {

BlendBinder::BlendBinder(Functions &functions, const BlendModeEnum blend)
    : m_functions{functions}
    , m_blend{blend}
{
    switch (m_blend) {
    case BlendModeEnum::NONE:
        m_functions.glDisable(GL_BLEND);
        break;
    case BlendModeEnum::TRANSPARENCY:
        m_functions.glEnable(GL_BLEND);
        m_functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BlendModeEnum::MODULATE:
        m_functions.glEnable(GL_BLEND);
        m_functions.glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
        break;
    }
}

BlendBinder::~BlendBinder()
{
    switch (m_blend) {
    case BlendModeEnum::NONE:
    case BlendModeEnum::TRANSPARENCY:
        break;
    case BlendModeEnum::MODULATE:
        m_functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
    m_functions.glDisable(GL_BLEND);
}

CullingBinder::CullingBinder(Functions &functions, const CullingEnum &culling)
    : m_functions{functions}
{
    switch (culling) {
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
    m_functions.glCullFace(mode);
    m_functions.glEnable(GL_CULL_FACE);
}

void CullingBinder::disable()
{
    m_functions.glDisable(GL_CULL_FACE);
    m_functions.glCullFace(GL_BACK);
}

DepthBinder::DepthBinder(Functions &functions, const DepthBinder::OptDepth &depth)
    : m_functions{functions}
    , m_depth{depth}
{
    if (m_depth.has_value()) {
        m_functions.glEnable(GL_DEPTH_TEST);
        m_functions.glDepthFunc(static_cast<GLenum>(m_depth.value()));
    } else {
        m_functions.glDisable(GL_DEPTH_TEST);
    }
}

DepthBinder::~DepthBinder()
{
    if (m_depth.has_value()) {
        m_functions.glDisable(GL_DEPTH_TEST);
        m_functions.glDepthFunc(GL_LESS);
    }
}

LineParamsBinder::LineParamsBinder(Functions &functions, const LineParams &lineParams)
    : m_functions{functions}
    , m_lineParams{lineParams}
{
    m_functions.glLineWidth(m_lineParams.width);
}

LineParamsBinder::~LineParamsBinder()
{
    const auto &width = m_lineParams.width;
    const float ONE = 1.f;
    static_assert(sizeof(ONE) == sizeof(width));
    if (!utils::equals(&width, &ONE)) {
        m_functions.glLineWidth(ONE);
    }
}

PointSizeBinder::PointSizeBinder(Functions &functions, const std::optional<GLfloat> &pointSize)
    : m_functions{functions}
    , m_optPointSize{pointSize}
{
    if (m_optPointSize.has_value()) {
        m_functions.enableProgramPointSize(true);
    }
}

PointSizeBinder::~PointSizeBinder()
{
    if (m_optPointSize.has_value()) {
        m_functions.enableProgramPointSize(false);
    }
}

TexturesBinder::TexturesBinder(const TexLookup &lookup, const TexturesBinder::Textures &textures)
    : m_lookup{lookup}
    , m_textures{textures}
{
    for (size_t i = 0, size = m_textures.size(); i < size; ++i) {
        const auto id = m_textures[i];
        if (id == INVALID_MM_TEXTURE_ID) {
            continue;
        }

        // note: pointer to shared pointer
        if (const SharedMMTexture *const pShared = m_lookup.find(id)) {
            if (const SharedMMTexture &tex = *pShared) {
                tex->bind(static_cast<uint32_t>(i));
            }
        }
    }
}

TexturesBinder::~TexturesBinder()
{
    for (size_t i = 0, size = m_textures.size(); i < size; ++i) {
        const auto id = m_textures[i];
        if (id == INVALID_MM_TEXTURE_ID) {
            continue;
        }

        // note: pointer to shared pointer
        if (const SharedMMTexture *const pShared = m_lookup.find(id)) {
            if (const SharedMMTexture &tex = *pShared) {
                tex->release(static_cast<uint32_t>(i));
            }
        }
    }
}

RenderStateBinder::RenderStateBinder(Functions &functions,
                                     const TexLookup &texLookup,
                                     const GLRenderState &renderState)
    : m_blendBinder{functions, renderState.blend}
    , m_cullingBinder{functions, renderState.culling}
    , m_depthBinder{functions, renderState.depth}
    , m_lineParamsBinder{functions, renderState.lineParams}
    , m_pointSizeBinder{functions, renderState.uniforms.pointSize}
    , m_texturesBinder{texLookup, renderState.uniforms.textures}
{}

} // namespace Legacy
