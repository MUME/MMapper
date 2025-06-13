#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/utils.h"
#include "Legacy.h"

#include <optional>

namespace Legacy {

struct NODISCARD BlendBinder final
{
private:
    Functions &m_functions;
    const BlendModeEnum m_blend;

public:
    explicit BlendBinder(Functions &functions, BlendModeEnum blend);
    ~BlendBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(BlendBinder);
};

struct NODISCARD CullingBinder final
{
private:
    Functions &m_functions;

public:
    explicit CullingBinder(Functions &functions, const CullingEnum &culling);
    ~CullingBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(CullingBinder);

private:
    void enable(GLenum mode);
    void disable();
};

struct NODISCARD DepthBinder final
{
public:
    using OptDepth = GLRenderState::OptDepth;

private:
    Functions &m_functions;
    const OptDepth &m_depth;

public:
    explicit DepthBinder(Functions &functions, const OptDepth &depth);
    ~DepthBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(DepthBinder);
};

struct NODISCARD LineParamsBinder
{
private:
    Functions &m_functions;
    const LineParams &m_lineParams;

public:
    explicit LineParamsBinder(Functions &functions, const LineParams &lineParams);
    ~LineParamsBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(LineParamsBinder);
};

struct NODISCARD PointSizeBinder
{
private:
    Functions &m_functions;
    const std::optional<GLfloat> &m_optPointSize;

public:
    explicit PointSizeBinder(Functions &functions, const std::optional<GLfloat> &pointSize);
    ~PointSizeBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(PointSizeBinder);
};

struct NODISCARD TexturesBinder final
{
public:
    using Textures = GLRenderState::Textures;

private:
    const TexLookup &m_lookup;
    const Textures &m_textures;

public:
    explicit TexturesBinder(const TexLookup &lookup, const Textures &textures);
    ~TexturesBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(TexturesBinder);
};

struct NODISCARD RenderStateBinder final
{
private:
    BlendBinder m_blendBinder;
    CullingBinder m_cullingBinder;
    DepthBinder m_depthBinder;
    LineParamsBinder m_lineParamsBinder;
    PointSizeBinder m_pointSizeBinder;
    TexturesBinder m_texturesBinder;

public:
    explicit RenderStateBinder(Functions &functions,
                               const TexLookup &,
                               const GLRenderState &renderState);
    DELETE_CTORS_AND_ASSIGN_OPS(RenderStateBinder);
    DTOR(RenderStateBinder) = default;
};

} // namespace Legacy
