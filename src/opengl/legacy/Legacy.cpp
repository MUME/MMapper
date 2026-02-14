// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Legacy.h"

#include "../../display/Textures.h"
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QDebug>
#include <QFile>
#include <QMessageLogContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>

namespace Legacy {

const char *Functions::getUniformBlockName(const SharedVboEnum block)
{
    switch (block) {
#define X_CASE(EnumName, StringName, isUniform) \
    case SharedVboEnum::EnumName: \
        if constexpr (isUniform) { \
            return StringName; \
        } \
        return nullptr;
        XFOREACH_SHARED_VBO(X_CASE)
#undef X_CASE
    }
    return nullptr;
}

void Functions::virt_glUniformBlockBinding(const GLuint program, const SharedVboEnum block)
{
    const char *const block_name = getUniformBlockName(block);
    if (block_name == nullptr) {
        return;
    }
    const GLuint block_index = Base::glGetUniformBlockIndex(program, block_name);
    if (block_index != GL_INVALID_INDEX) {
        Base::glUniformBlockBinding(program, block_index, static_cast<GLuint>(block));
    }
}

void Functions::applyDefaultUniformBlockBindings(const GLuint program)
{
#define X_BIND(EnumName, StringName, isUniform) \
    if constexpr (isUniform) { \
        virt_glUniformBlockBinding(program, SharedVboEnum::EnumName); \
    }
    XFOREACH_SHARED_VBO(X_BIND)
#undef X_BIND
}

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
                                               const MMTextureId texture)
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
                                          const MMTextureId texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedUColorShader();
    return createTexturedMesh<TexturedMesh>(shared_from_this(), mode, batch, prog, texture);
}

UniqueMesh Functions::createColoredTexturedBatch(const DrawModeEnum mode,
                                                 const std::vector<ColoredTexVert> &batch,
                                                 const MMTextureId texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedAColorShader();
    return createTexturedMesh<ColoredTexturedMesh>(shared_from_this(), mode, batch, prog, texture);
}

UniqueMesh Functions::createRoomQuadTexBatch(const std::vector<RoomQuadTexVert> &batch,
                                             const MMTextureId texture)
{
    const auto mode = DrawModeEnum::INSTANCED_QUADS;
    const auto &prog = getShaderPrograms().getRoomQuadTexShader();
    return createTexturedMesh<RoomQuadTexMesh>(shared_from_this(), mode, batch, prog, texture);
}

template<typename VertexType_, template<typename> typename Mesh_, typename ShaderType_>
static void renderImmediate(const SharedFunctions &sharedFunctions,
                            const DrawModeEnum mode,
                            const std::vector<VertexType_> &verts,
                            const std::shared_ptr<ShaderType_> &sharedShader,
                            const GLRenderState &renderState)
{
    if (verts.empty()) {
        return;
    }

    static WeakVbo weak;
    auto shared = weak.lock();
    if (shared == nullptr) {
        weak = shared = sharedFunctions->getStaticVbos().alloc();
        if (shared == nullptr) {
            throw std::runtime_error("OpenGL error: failed to alloc VBO");
        }
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
                           .withTexture0(texture->getId());

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

Functions::Functions(Badge<Functions>)
    : m_shaderPrograms{std::make_unique<ShaderPrograms>(*this)}
    , m_staticVbos{std::make_unique<StaticVbos>()}
    , m_sharedVbos{std::make_unique<SharedVbos>()}
    , m_sharedVaos{std::make_unique<SharedVaos>()}
    , m_texLookup{std::make_unique<TexLookup>()}
    , m_fbo{std::make_unique<FBO>()}
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
    getSharedVbos().resetAll();
    getSharedVaos().resetAll();
    getTexLookup().clear();
}

ShaderPrograms &Functions::getShaderPrograms()
{
    return deref(m_shaderPrograms);
}
StaticVbos &Functions::getStaticVbos()
{
    return deref(m_staticVbos);
}
SharedVbos &Functions::getSharedVbos()
{
    return deref(m_sharedVbos);
}
SharedVaos &Functions::getSharedVaos()
{
    return deref(m_sharedVaos);
}
TexLookup &Functions::getTexLookup()
{
    return deref(m_texLookup);
}
FBO &Functions::getFBO()
{
    return deref(m_fbo);
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
#ifdef __EMSCRIPTEN__
        // On WASM/WebGL, don't abort on GL errors - just log them.
        // WebGL can generate errors in cases that work fine, and aborting
        // makes debugging impossible.
        qWarning() << "OpenGL error detected (WASM mode - continuing execution)";
#else
        std::abort();
#endif
    }

#undef CASE
}

int Functions::clearErrors()
{
    int count = 0;
    while (Base::glGetError() != GL_NO_ERROR) {
        ++count;
    }
    return count;
}

void Functions::configureFbo(int samples)
{
    getFBO().configure(getPhysicalViewport(), samples);
}

void Functions::bindFbo()
{
    getFBO().bind();
}

void Functions::releaseFbo()
{
    getFBO().release();
}

void Functions::blitFboToDefault()
{
    getFBO().resolve();

    const GLuint textureId = getFBO().resolvedTextureId();
    if (textureId != 0) {
        const auto state = GLRenderState()
                               .withBlend(BlendModeEnum::NONE)
                               .withDepthFunction(std::nullopt)
                               .withCulling(CullingEnum::DISABLED);

        Base::glActiveTexture(GL_TEXTURE0);
        Base::glBindTexture(GL_TEXTURE_2D, textureId);

        renderFullScreenTriangle(getShaderPrograms().getBlitShader(), state);

        Base::glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Functions::renderFullScreenTriangle(const std::shared_ptr<AbstractShaderProgram> &prog,
                                         const GLRenderState &state)
{
    checkError();

    auto programUnbinder = prog->bind();
    prog->setUniforms(glm::mat4(1.0f), state.uniforms);
    RenderStateBinder renderStateBinder(*this, getTexLookup(), state);

    SharedVao shared = getSharedVaos().get(SharedVaoEnum::EmptyVao);
    VAO &vao = deref(shared);
    if (!vao) {
        if (IS_DEBUG_BUILD) {
            qDebug() << "allocating shared empty VAO for renderFullScreenTriangle";
        }
        vao.emplace(shared_from_this());
    }
    glBindVertexArray(vao.get());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    checkError();
}

} // namespace Legacy
