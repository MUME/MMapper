// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Legacy.h"

#include "../../display/Textures.h"
#include "../../global/utils.h"
#include "../OpenGLTypes.h"
#include "../UboManager.h"
#include "AbstractShaderProgram.h"
#include "Binders.h"
#include "FontMesh3d.h"
#include "Meshes.h"
#include "ShaderUtils.h"
#include "Shaders.h"
#include "SimpleMesh.h"
#include "TFO.h"
#include "VAO.h"
#include "VBO.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QDebug>
#include <QFile>
#include <QMessageLogContext>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>

namespace Legacy {

const char *Functions::getUniformBlockName(const SharedVboEnum block)
{
    switch (block) {
#define X_CASE(EnumName, StringName) \
    case SharedVboEnum::EnumName: \
        return StringName;
        XFOREACH_SHARED_VBO(X_CASE)
#undef X_CASE
    case SharedVboEnum::NUM_BLOCKS:
        break;
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
#define X_BIND(EnumName, StringName) virt_glUniformBlockBinding(program, SharedVboEnum::EnumName);
    XFOREACH_SHARED_VBO(X_BIND)
#undef X_BIND
}

template<template<typename> typename Mesh_, typename VertType_, typename ProgType_>
NODISCARD static auto createMesh(const SharedFunctions &functions,
                                 const DrawModeEnum mode,
                                 const View<VertType_> batch,
                                 const std::shared_ptr<ProgType_> &prog)
{
    using Mesh = Mesh_<VertType_>;
    static_assert(std::is_same_v<typename Mesh::ProgramType, ProgType_>);
    return std::make_unique<Mesh>(functions, prog, mode, batch);
}

template<template<typename> typename Mesh_, typename VertType_, typename ProgType_>
NODISCARD static UniqueMesh createUniqueMesh(const SharedFunctions &functions,
                                             const DrawModeEnum mode,
                                             const View<VertType_> batch,
                                             const std::shared_ptr<ProgType_> &prog)
{
    assert(mode != DrawModeEnum::INVALID);
    return UniqueMesh(createMesh<Mesh_, VertType_, ProgType_>(functions, mode, batch, prog));
}

template<template<typename> typename Mesh_, typename VertType_, typename ProgType_>
NODISCARD static UniqueMesh createTexturedMesh(const SharedFunctions &functions,
                                               const DrawModeEnum mode,
                                               const View<VertType_> batch,
                                               const std::shared_ptr<ProgType_> &prog,
                                               const MMTextureId texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    return UniqueMesh{std::make_unique<TexturedRenderable>(
        texture, createMesh<Mesh_, VertType_, ProgType_>(functions, mode, batch, prog))};
}

UniqueMesh Functions::createPointBatch(const View<ColorVert> batch)
{
    const auto &prog = getShaderPrograms().getPointShader();
    return createUniqueMesh<PointMesh>(shared_from_this(), DrawModeEnum::POINTS, batch, prog);
}

UniqueMesh Functions::createPlainLineBatch(View<glm::vec3> batch)
{
    const auto &prog = getShaderPrograms().getLineUColorShader();
    return createUniqueMesh<LineMesh>(shared_from_this(), DrawModeEnum::LINES, batch, prog);
}

UniqueMesh Functions::createColoredLineBatch(View<ColorVert> batch)
{
    const auto &prog = getShaderPrograms().getLineAColorShader();
    return createUniqueMesh<ColoredLineMesh>(shared_from_this(), DrawModeEnum::LINES, batch, prog);
}

UniqueMesh Functions::createPlainBatch(const DrawModeEnum mode, const View<glm::vec3> batch)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getPlainUColorShader();
    return createUniqueMesh<PlainMesh>(shared_from_this(), mode, batch, prog);
}

UniqueMesh Functions::createColoredBatch(const DrawModeEnum mode, const View<ColorVert> batch)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getPlainAColorShader();
    return createUniqueMesh<ColoredMesh>(shared_from_this(), mode, batch, prog);
}

UniqueMesh Functions::createTexturedBatch(const DrawModeEnum mode,
                                          const View<TexVert> batch,
                                          const MMTextureId texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedUColorShader();
    return createTexturedMesh<TexturedMesh>(shared_from_this(), mode, batch, prog, texture);
}

UniqueMesh Functions::createColoredTexturedBatch(const DrawModeEnum mode,
                                                 const View<ColoredTexVert> batch,
                                                 const MMTextureId texture)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedAColorShader();
    return createTexturedMesh<ColoredTexturedMesh>(shared_from_this(), mode, batch, prog, texture);
}

UniqueMesh Functions::createRoomQuadTexBatch(const View<RoomQuadTexVert> batch,
                                             const MMTextureId texture)
{
    const auto mode = DrawModeEnum::INSTANCED_QUADS;
    const auto &prog = getShaderPrograms().getRoomQuadTexShader();
    return createTexturedMesh<RoomQuadTexMesh>(shared_from_this(), mode, batch, prog, texture);
}

namespace {

template<typename T>
NODISCARD std::shared_ptr<T> alloc(Functions &) = delete;
template<>
NODISCARD std::shared_ptr<VAO> alloc(Functions &f)
{
    return f.getStaticVaos().alloc();
}
template<>
NODISCARD std::shared_ptr<VBO> alloc(Functions &f)
{
    return f.getStaticVbos().alloc();
}

#define XFOREACH_VXO(X) X(VAO) X(VBO)

template<typename T>
NODISCARD std::string getTypeName() = delete;

#define X_DECL_WHAT(_x) \
    template<> \
    NODISCARD std::string getTypeName<_x>() \
    { \
        return #_x; \
    }
XFOREACH_VXO(X_DECL_WHAT)
#undef X_DECL_WHAT
#undef XFOREACH_VXO

template<typename T>
NODISCARD T &get(const SharedFunctions &sharedFunctions, std::weak_ptr<T> &weak)
{
    auto shared = weak.lock();
    if (shared == nullptr) {
        weak = shared = alloc<T>(deref(sharedFunctions));
        if (shared == nullptr) {
            throw std::runtime_error("OpenGL error: failed to alloc " + getTypeName<T>());
        }
    }

    T &x = *shared;
    if (!x.isValid()) {
        x.emplace(sharedFunctions);
    }

    return x;
}

} // namespace

template<typename VertexType_, template<typename> typename Mesh_, typename ShaderType_>
static void renderImmediate(const SharedFunctions &sharedFunctions,
                            const DrawModeEnum mode,
                            const View<VertexType_> verts,
                            const std::shared_ptr<ShaderType_> &sharedShader,
                            const GLRenderState &renderState)
{
    if (verts.empty()) {
        return;
    }

    static WeakVao weakVao;
    VAO &vao = get(sharedFunctions, weakVao);

    static WeakVbo weakVbo;
    VBO &vbo = get(sharedFunctions, weakVbo);

    using Mesh = Mesh_<VertexType_>;
    static_assert(std::is_same_v<typename Mesh::ProgramType, ShaderType_>);

    const auto vaoBefore = vao.get();
    const auto vboBefore = vbo.get();
    {
        Mesh mesh{sharedFunctions, sharedShader, DoNotAllocateVao{}};
        {
            // temporarily loan our VBO to the mesh.
            mesh.unsafe_swapVaoId(vao);
            mesh.unsafe_swapVboId(vbo);
            assert(!vao.isValid());
            assert(!vbo.isValid());
            {
                mesh.setDynamic(mode, verts);
                mesh.render(renderState);
            }
            mesh.unsafe_swapVaoId(vao);
            mesh.unsafe_swapVboId(vbo);
            assert(vao.isValid());
            assert(vbo.isValid());
        }
    }
    const auto vaoAfter = vao.get();
    const auto vboAfter = vbo.get();
    assert(vaoBefore == vaoAfter);
    assert(vboBefore == vboAfter);

    vbo.clearVbo();
}

void Functions::renderPlainLines(const View<glm::vec3> verts, const GLRenderState &state)
{
    const auto &shared = shared_from_this();
    const auto &prog = getShaderPrograms().getLineUColorShader();
    renderImmediate<glm::vec3, Legacy::LineMesh>(shared, DrawModeEnum::LINES, verts, prog, state);
}

void Functions::renderColoredLines(const View<ColorVert> verts, const GLRenderState &state)
{
    const auto &shared = shared_from_this();
    const auto &prog = getShaderPrograms().getLineAColorShader();
    renderImmediate<ColorVert, Legacy::ColoredLineMesh>(shared,
                                                        DrawModeEnum::LINES,
                                                        verts,
                                                        prog,
                                                        state);
}

void Functions::renderPlain(const DrawModeEnum mode,
                            const View<glm::vec3> verts,
                            const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &shared = shared_from_this();
    const auto &prog = getShaderPrograms().getPlainUColorShader();
    renderImmediate<glm::vec3, Legacy::PlainMesh>(shared, mode, verts, prog, state);
}

void Functions::renderColored(const DrawModeEnum mode,
                              const View<ColorVert> verts,
                              const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getPlainAColorShader();
    renderImmediate<ColorVert, Legacy::ColoredMesh>(shared_from_this(), mode, verts, prog, state);
}

void Functions::renderPoints(const View<ColorVert> verts, const GLRenderState &state)
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
                               const View<TexVert> verts,
                               const GLRenderState &state)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getTexturedUColorShader();
    renderImmediate<TexVert, Legacy::TexturedMesh>(shared_from_this(), mode, verts, prog, state);
}

void Functions::renderColoredTextured(const DrawModeEnum mode,
                                      const View<ColoredTexVert> verts,
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

void Functions::renderFont3d(const SharedMMTexture &texture, const View<FontVert3d> verts)

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
                                     const View<FontVert3d> batch)
{
    assert(static_cast<size_t>(mode) >= VERTS_PER_TRI);
    const auto &prog = getShaderPrograms().getFontShader();
    return UniqueMesh{
        std::make_unique<Legacy::FontMesh3d>(shared_from_this(), prog, texture, mode, batch)};
}

Functions::Functions(Badge<Functions>, UboManager &uboManager)
    : m_shaderPrograms{std::make_unique<ShaderPrograms>(*this)}
    , m_sharedVaos{std::make_unique<SharedVaos>()}
    , m_sharedVbos{std::make_unique<SharedVbos>()}
    , m_staticVaos{std::make_unique<StaticVaos>()}
    , m_staticVbos{std::make_unique<StaticVbos>()}
    , m_texLookup{std::make_unique<TexLookup>()}
    , m_fbo{std::make_unique<FBO>()}
    , m_uboManager{uboManager}
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
    if (IS_DEBUG_BUILD) {
        qInfo() << "Cleanup";
    }

    getShaderPrograms().resetAll();
    getSharedVaos().resetAll();
    getSharedVbos().resetAll();
    getStaticVaos().resetAll();
    getStaticVbos().resetAll();
    getTexLookup().clear();
    m_fbo.reset();
}

ShaderPrograms &Functions::getShaderPrograms()
{
    return deref(m_shaderPrograms);
}
SharedVaos &Functions::getSharedVaos()
{
    return deref(m_sharedVaos);
}
SharedVbos &Functions::getSharedVbos()
{
    return deref(m_sharedVbos);
}
StaticVaos &Functions::getStaticVaos()
{
    return deref(m_staticVaos);
}
StaticVbos &Functions::getStaticVbos()
{
    return deref(m_staticVbos);
}
TexLookup &Functions::getTexLookup()
{
    return deref(m_texLookup);
}
FBO &Functions::getFBO()
{
    return deref(m_fbo);
}

UboManager &Functions::getUboManager()
{
    return m_uboManager;
}

void Functions::enableAttrib(const GLuint index,
                             const GLint size,
                             const GLenum type,
                             const GLboolean normalized,
                             const GLsizei stride,
                             const size_t offset)
{
    const auto pointer = reinterpret_cast<const GLvoid *>(offset);
    Base::glEnableVertexAttribArray(index);
    Base::glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    checkError();
}

void Functions::enableAttribI(const GLuint index,
                              const GLint size,
                              const GLenum type,
                              const GLsizei stride,
                              const size_t offset)
{
    const auto pointer = reinterpret_cast<const GLvoid *>(offset);
    Base::glEnableVertexAttribArray(index);
    Base::glVertexAttribIPointer(index, size, type, stride, pointer);
    checkError();
}

/// This only exists so we can detect errors in contexts that don't support \c glDebugMessageCallback().
void Functions::checkError()
{
    // glGetError() returns GL_INVALID_OPERATION indefinitely when called without a current context
    // (e.g. on NVIDIA drivers), causing an infinite loop. Skip the check if no context is current.
    if (QOpenGLContext::currentContext() == nullptr) {
        return;
    }

    static auto reportError = [](const auto what) { //
        qCritical() << "OpenGL error" << what;
    };

    static auto tryReportError = [](const auto err) {
#define X_CASE(_x) \
    case (_x): \
        return reportError(#_x)
        switch (err) {
            // these report the user-readable names
            X_CASE(GL_INVALID_ENUM);
            X_CASE(GL_INVALID_VALUE);
            X_CASE(GL_INVALID_OPERATION);
            X_CASE(GL_OUT_OF_MEMORY);
        default:
            // this just reports the integer error number
            return reportError(err);
        }
#undef X_CASE
    };

    // We don't expect opengl to ever report more than a handful of errors at a time, so this
    // arbitrary_limit is just to avoid an infinite loop in the rare case where opengl starts
    // returning the same value forever (e.g. if the context is null, which we check above).
    //
    // Note: fail will be set to true unless the first value is GL_NO_ERROR, so we'll abort
    // anyway once the arbitrary limit is reached. The only reason to increase the number
    // would be some odd case where opengl somehow legitimately reports that many errors
    // -AND- we need to also see the last error message.
    //
    // Better solution: use an opengl 4.3+ synchronous glDebugMessageCallback() to get
    // immediate feedback at the call site instead of using the legacy glGetError() function.
    //
    constexpr size_t arbitrary_limit = 20;
    bool fail = false;
    for (size_t i = 0; i < arbitrary_limit; ++i) {
        if (const auto err = Base::glGetError(); err == GL_NO_ERROR) {
            break;
        } else {
            fail = true;
            tryReportError(err);
        }
    }

    if (fail) {
        std::abort();
    }
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

    if (const GLuint textureId = getFBO().resolvedTextureId(); textureId != 0) {
        const auto state = GLRenderState()
                               .withBlend(BlendModeEnum::NONE)
                               .withDepthFunction(std::nullopt)
                               .withCulling(CullingEnum::DISABLED);

        MAYBE_UNUSED auto texture_binder = bindTexture0(GL_TEXTURE_2D, textureId);
        renderFullScreenTriangle(getShaderPrograms().getBlitShader(), state);
    }
}

void Functions::renderFullScreenTriangle(const std::shared_ptr<AbstractShaderProgram> &prog_ptr,
                                         const GLRenderState &state)
{
    checkError();
    auto &prog = deref(prog_ptr);
    MAYBE_UNUSED auto program_binder = prog.bind();
    prog.setUniforms(glm::mat4(1.0f), state.uniforms);

    MAYBE_UNUSED auto renderStateBinder = RenderStateBinder{*this, getTexLookup(), state};

    SharedVao shared = getSharedVaos().get(SharedVaoEnum::EmptyVao);
    VAO &vao = deref(shared);
    if (!vao.isValid()) {
        if (IS_DEBUG_BUILD) {
            qDebug() << "allocating shared empty VAO for renderFullScreenTriangle";
        }
        vao.emplace(shared_from_this());
    }

    MAYBE_UNUSED auto vao_binder = vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkError();
}

} // namespace Legacy
