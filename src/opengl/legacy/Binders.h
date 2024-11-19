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
    Functions &functions;
    const BlendModeEnum blend;

public:
    explicit BlendBinder(Functions &in_functions, BlendModeEnum in_blend);
    ~BlendBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(BlendBinder);
};

struct NODISCARD CullingBinder final
{
private:
    Functions &functions;

public:
    explicit CullingBinder(Functions &in_functions, const CullingEnum &in_culling);
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
    Functions &functions;
    const OptDepth &depth;

public:
    explicit DepthBinder(Functions &in_functions, const OptDepth &in_depth);
    ~DepthBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(DepthBinder);
};

struct NODISCARD LineParamsBinder
{
private:
    Functions &functions;
    const LineParams &lineParams;

public:
    explicit LineParamsBinder(Functions &in_functions, const LineParams &in_lineParams);
    ~LineParamsBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(LineParamsBinder);
};

struct NODISCARD PointSizeBinder
{
private:
    Functions &functions;
    const std::optional<GLfloat> &optPointSize;

public:
    explicit PointSizeBinder(Functions &in_functions, const std::optional<GLfloat> &in_pointSize);
    ~PointSizeBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(PointSizeBinder);
};

struct NODISCARD TexturesBinder final
{
public:
    using Textures = GLRenderState::Textures;

private:
    const TexLookup &lookup;
    const Textures &textures;

public:
    explicit TexturesBinder(const TexLookup &in_lookup, const Textures &in_textures);
    ~TexturesBinder();
    DELETE_CTORS_AND_ASSIGN_OPS(TexturesBinder);
};

struct NODISCARD RenderStateBinder final
{
private:
    BlendBinder blendBinder;
    CullingBinder cullingBinder;
    DepthBinder depthBinder;
    LineParamsBinder lineParamsBinder;
    PointSizeBinder pointSizeBinder;
    TexturesBinder texturesBinder;

public:
    explicit RenderStateBinder(Functions &functions,
                               const TexLookup &,
                               const GLRenderState &renderState);
    DELETE_CTORS_AND_ASSIGN_OPS(RenderStateBinder);
    DTOR(RenderStateBinder) = default;
};

} // namespace Legacy
