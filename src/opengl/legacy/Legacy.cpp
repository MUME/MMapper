// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Legacy.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <QDebug>
#include <QFile>
#include <QMessageLogContext>
#include <QOpenGLTexture>

#include "../../global/utils.h"
#include "../OpenGLTypes.h"
#include "AbstractShaderProgram.h"
#include "Binders.h"
#include "FontMesh3d.h"
#include "Meshes.h"
#include "ShaderUtils.h"
#include "Shaders.h"
#include "SimpleMesh.h"
#include "VBO.h"

namespace Legacy {

template<template<typename> typename Mesh_, typename VertType_, typename ProgType_>
NODISCARD static auto createMesh(const SharedFunctions &functions,
                                 const DrawModeEnum mode,
                                 const std::vector<VertType_> &batch,
                                 const std::shared_ptr<ProgType_> &prog)
{
    using Mesh = Mesh_<VertType_>;
    static_assert(std::is_same_v<typename Mesh::ProgramType, ProgType_>);
    return std::make_unique<Mesh>(functions, prog, mode, batch);
}

template<template<typename> typename Mesh_, typename VertType_, typename ProgType_>
NODISCARD static UniqueMesh createUniqueMesh(const SharedFunctions &functions,
                                             const DrawModeEnum mode,
                                             const std::vector<VertType_> &batch,
                                             const std::shared_ptr<ProgType_> &prog)
{
    assert(mode != DrawModeEnum::INVALID);
    return UniqueMesh(createMesh<Mesh_, VertType_, ProgType_>(functions, mode, batch, prog));
}

template<template<typename> typename Mesh_, typename VertType_, typename ProgType_>
NODISCARD static UniqueMesh createTexturedMesh(const SharedFunctions &functions,
                                               const DrawModeEnum mode,
                                               const std::vector<VertType_> &batch,
                                               const std::shared_ptr<ProgType_> &prog,
                                               const SharedMMTexture &texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    return UniqueMesh{std::make_unique<TexturedRenderable>(
        texture, createMesh<Mesh_, VertType_, ProgType_>(functions, mode, batch, prog))};
}

UniqueMesh Functions::createPointBatch(const std::vector<ColorVert> &batch)
{
    const auto &prog = getShaderPrograms().getPointShader();
    return createUniqueMesh<PointMesh>(shared_from_this(), DrawModeEnum::POINTS, batch, prog);
}

UniqueMesh Functions::createPlainBatch(const DrawModeEnum mode, const std::vector<glm::vec3> &batch)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_LINE);
    const auto &prog = getShaderPrograms().getPlainUColorShader();
    return createUniqueMesh<PlainMesh>(shared_from_this(), mode, batch, prog);
}

UniqueMesh Functions::createColoredBatch(const DrawModeEnum mode,
                                         const std::vector<ColorVert> &batch)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_LINE);
    const auto &prog = getShaderPrograms().getPlainAColorShader();
    return createUniqueMesh<ColoredMesh>(shared_from_this(), mode, batch, prog);
}

UniqueMesh Functions::createTexturedBatch(const DrawModeEnum mode,
                                          const std::vector<TexVert> &batch,
                                          const SharedMMTexture &texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedUColorShader();
    return createTexturedMesh<TexturedMesh>(shared_from_this(), mode, batch, prog, texture);
}

UniqueMesh Functions::createColoredTexturedBatch(const DrawModeEnum mode,
                                                 const std::vector<ColoredTexVert> &batch,
                                                 const SharedMMTexture &texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedAColorShader();
    return createTexturedMesh<ColoredTexturedMesh>(shared_from_this(), mode, batch, prog, texture);
}

template<typename VertexType_, template<typename> typename Mesh_, typename ShaderType_>
static void renderImmediate(const SharedFunctions &sharedFunctions,
                            const DrawModeEnum mode,
                            const std::vector<VertexType_> &verts,
                            const std::shared_ptr<ShaderType_> &sharedShader,
                            const GLRenderState &renderState)
{
    if (verts.empty())
        return;

    static WeakVbo weak;
    auto shared = weak.lock();
    if (shared == nullptr) {
        weak = shared = sharedFunctions->getStaticVbos().alloc();
        if (shared == nullptr)
            throw std::runtime_error("OpenGL error: failed to alloc VBO");
    }

    VBO &vbo = *shared;
    if (!vbo) {
        vbo.emplace(sharedFunctions);
    }

    using Mesh = Mesh_<VertexType_>;
    static_assert(std::is_same_v<typename Mesh::ProgramType, ShaderType_>);

    const auto before = vbo.get();
    {
        Mesh mesh{sharedFunctions, sharedShader};
        {
            // temporarily loan our VBO to the mesh.
            mesh.unsafe_swapVboId(vbo);
            assert(!vbo);
            {
                mesh.setDynamic(mode, verts);
                mesh.render(renderState);
            }
            mesh.unsafe_swapVboId(vbo);
            assert(vbo);
        }
    }
    const auto after = vbo.get();
    assert(before == after);
    sharedFunctions->clearVbo(vbo.get());
}

void Functions::renderPlain(const DrawModeEnum mode,
                            const std::vector<glm::vec3> &verts,
                            const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_LINE);
    const auto &shared = shared_from_this();
    const auto &prog = getShaderPrograms().getPlainUColorShader();
    renderImmediate<glm::vec3, Legacy::PlainMesh>(shared, mode, verts, prog, state);
}

void Functions::renderColored(const DrawModeEnum mode,
                              const std::vector<ColorVert> &verts,
                              const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_LINE);
    const auto &prog = getShaderPrograms().getPlainAColorShader();
    renderImmediate<ColorVert, Legacy::ColoredMesh>(shared_from_this(), mode, verts, prog, state);
}

void Functions::renderPoints(const std::vector<ColorVert> &verts, const GLRenderState &state)
{
    assert(state.uniforms.pointSize);
    const auto &prog = getShaderPrograms().getPointShader();
    renderImmediate<ColorVert, Legacy::PointMesh>(shared_from_this(),
                                                  DrawModeEnum::POINTS,
                                                  verts,
                                                  prog,
                                                  state);
}

void Functions::renderTextured(const DrawModeEnum mode,
                               const std::vector<TexVert> &verts,
                               const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedUColorShader();
    renderImmediate<TexVert, Legacy::TexturedMesh>(shared_from_this(), mode, verts, prog, state);
}

void Functions::renderColoredTextured(const DrawModeEnum mode,
                                      const std::vector<ColoredTexVert> &verts,
                                      const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedAColorShader();
    renderImmediate<ColoredTexVert, Legacy::ColoredTexturedMesh>(shared_from_this(),
                                                                 mode,
                                                                 verts,
                                                                 prog,
                                                                 state);
}

void Functions::renderFont3d(const SharedMMTexture &texture, const std::vector<FontVert3d> &verts)

{
    const auto state = GLRenderState()
                           .withBlend(BlendModeEnum::TRANSPARENCY)
                           .withDepthFunction(std::nullopt)
                           .withTexture0(texture);

    const auto &prog = getShaderPrograms().getFontShader();
    renderImmediate<FontVert3d, Legacy::SimpleFont3dMesh>(shared_from_this(),
                                                          DrawModeEnum::QUADS,
                                                          verts,
                                                          prog,
                                                          state);
}

UniqueMesh Functions::createFontMesh(const SharedMMTexture &texture,
                                     const DrawModeEnum mode,
                                     const std::vector<FontVert3d> &batch)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getFontShader();
    return UniqueMesh{
        std::make_unique<Legacy::FontMesh3d>(shared_from_this(), prog, texture, mode, batch)};
}

Functions::Functions(this_is_private)
    : m_shaderPrograms{std::make_unique<ShaderPrograms>(*this)}
    , m_staticVbos{std::make_unique<StaticVbos>()}
{}

Functions::~Functions()
{
    cleanup();
}

/// <ul>
/// <li>Resets the Wrapped GL's cached copies of (compiled) shaders given out
/// to new meshes. This <em>does NOT</em> expire the shaders belonging to old
/// mesh objects, so that means it's possible to end up with a mixture of old
/// and new mesh objects each with different instances of the same shader
/// program. (In other words: if you want to add shader hot-reloading, then
/// instead of calling this function you'll probably want to just immediately
/// recompile the old shaders.) </li>
///
/// <li>Resets shared pointers to VBOs owned by this object but given out on
/// "extended loan" to static immediate-rendering functions. Those functions
/// only keep static weak pointers to the VBOs, and the weak pointers will
/// expire immediately when you call this function. If you call those
/// functions again, they'll detect the expiration and request new buffers.</li>
/// </ul>
void Functions::cleanup()
{
    if (LOG_VBO_ALLOCATIONS) {
        qInfo() << "Cleanup";
    }

    getShaderPrograms().resetAll();
    getStaticVbos().resetAll();
}

ShaderPrograms &Functions::getShaderPrograms()
{
    return deref(m_shaderPrograms);
}
StaticVbos &Functions::getStaticVbos()
{
    return deref(m_staticVbos);
}

std::shared_ptr<Functions> Functions::alloc()
{
    return std::make_shared<Functions>(this_is_private{0});
}

/// This only exists so we can detect errors in contexts that don't support \c glDebugMessageCallback().
void Functions::checkError()
{
#define CASE(x) \
    case (x): \
        qCritical() << "OpenGL error" << #x; \
        break;

    bool fail = false;
    while (true) {
        const auto err = Base::glGetError();
        if (err == GL_NO_ERROR) {
            break;
        }

        fail = true;

        switch (err) {
            CASE(GL_INVALID_ENUM)
            CASE(GL_INVALID_VALUE)
            CASE(GL_INVALID_OPERATION)
            CASE(GL_OUT_OF_MEMORY)
        default:
            qCritical() << "OpenGL error" << err;
            break;
        }
    }

    if (fail) {
        std::abort();
    }

#undef CASE
}

} // namespace Legacy
