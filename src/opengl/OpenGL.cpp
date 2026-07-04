// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "OpenGL.h"

#include "../display/Textures.h"
#include "../global/ConfigConsts.h"
#include "../global/logging.h"
#include "./legacy/FunctionsES30.h"
#include "./legacy/FunctionsGL33.h"
#include "./legacy/Legacy.h"
#include "./legacy/Meshes.h"
#include "./legacy/VBO.h"
#include "OpenGLConfig.h"
#include "OpenGLProber.h"
#include "OpenGLTypes.h"

#include <cassert>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLContext>
#include <QSurfaceFormat>

OpenGL::OpenGL()
{
    switch (OpenGLConfig::getBackendType()) {
    case OpenGLProber::BackendType::GL:
        m_opengl = Legacy::Functions::alloc<Legacy::FunctionsGL33>(m_uboManager);
        break;
    case OpenGLProber::BackendType::GLES:
        m_opengl = Legacy::Functions::alloc<Legacy::FunctionsES30>(m_uboManager);
        break;
    case OpenGLProber::BackendType::None:
    default:
        qFatal("Invalid backend type");
        break;
    }
}

OpenGL::~OpenGL() = default;

glm::mat4 OpenGL::getProjectionMatrix() const
{
    return getFunctions().getProjectionMatrix();
}

Viewport OpenGL::getViewport() const
{
    return getFunctions().getViewport();
}

Viewport OpenGL::getPhysicalViewport() const
{
    return getFunctions().getPhysicalViewport();
}

void OpenGL::setProjectionMatrix(const glm::mat4 &m)
{
    getFunctions().setProjectionMatrix(m);
}

void OpenGL::configureFbo(int samples)
{
    getFunctions().configureFbo(samples);
}

void OpenGL::bindFbo()
{
    getFunctions().bindFbo();
}

void OpenGL::releaseFbo()
{
    getFunctions().releaseFbo();
}

void OpenGL::blitFboToDefault()
{
    getFunctions().blitFboToDefault();
}

UniqueMesh OpenGL::createPointBatch(const View<ColorVert> batch)
{
    return getFunctions().createPointBatch(batch);
}

UniqueMesh OpenGL::createPlainLineBatch(const View<glm::vec3> batch)
{
    return getFunctions().createPlainLineBatch(batch);
}

UniqueMesh OpenGL::createColoredLineBatch(const View<ColorVert> batch)
{
    return getFunctions().createColoredLineBatch(batch);
}

UniqueMesh OpenGL::createPlainTriBatch(const View<glm::vec3> batch)
{
    return getFunctions().createPlainBatch(DrawModeEnum::TRIANGLES, batch);
}

UniqueMesh OpenGL::createColoredTriBatch(const View<ColorVert> batch)
{
    return getFunctions().createColoredBatch(DrawModeEnum::TRIANGLES, batch);
}

UniqueMesh OpenGL::createPlainQuadBatch(const View<glm::vec3> batch)
{
    return getFunctions().createPlainBatch(DrawModeEnum::QUADS, batch);
}

UniqueMesh OpenGL::createColoredQuadBatch(const View<ColorVert> batch)
{
    return getFunctions().createColoredBatch(DrawModeEnum::QUADS, batch);
}

UniqueMesh OpenGL::createTexturedQuadBatch(const View<TexVert> batch, const MMTextureId texture)
{
    return getFunctions().createTexturedBatch(DrawModeEnum::QUADS, batch, texture);
}

UniqueMesh OpenGL::createColoredTexturedQuadBatch(const View<ColoredTexVert> batch,
                                                  const MMTextureId texture)
{
    return getFunctions().createColoredTexturedBatch(DrawModeEnum::QUADS, batch, texture);
}

UniqueMesh OpenGL::createRoomQuadTexBatch(const View<RoomQuadTexVert> batch,
                                          const MMTextureId texture)
{
    return getFunctions().createRoomQuadTexBatch(batch, texture);
}

UniqueMesh OpenGL::createFontMesh(const SharedMMTexture &texture,
                                  const DrawModeEnum mode,
                                  const View<FontVert3d> batch)
{
    return getFunctions().createFontMesh(texture, mode, batch);
}

void OpenGL::clear(const Color color)
{
    const auto v = color.getVec4();
    auto &gl = getFunctions();
    gl.glClearColor(v.r, v.g, v.b, v.a);
    gl.glClear(static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void OpenGL::clearDepth()
{
    auto &gl = getFunctions();
    gl.glClear(static_cast<GLbitfield>(GL_DEPTH_BUFFER_BIT));
}

void OpenGL::renderPlain(const DrawModeEnum type,
                         const View<glm::vec3> verts,
                         const GLRenderState &state)
{
    if (type == DrawModeEnum::LINES) {
        getFunctions().renderPlainLines(verts, state);
    } else {
        getFunctions().renderPlain(type, verts, state);
    }
}

void OpenGL::renderColored(const DrawModeEnum type,
                           const View<ColorVert> verts,
                           const GLRenderState &state)
{
    if (type == DrawModeEnum::LINES) {
        getFunctions().renderColoredLines(verts, state);
    } else {
        getFunctions().renderColored(type, verts, state);
    }
}

void OpenGL::renderPoints(const View<ColorVert> verts, const GLRenderState &state)
{
    getFunctions().renderPoints(verts, state);
}

void OpenGL::renderTextured(const DrawModeEnum type,
                            const View<TexVert> verts,
                            const GLRenderState &state)
{
    getFunctions().renderTextured(type, verts, state);
}

void OpenGL::renderColoredTextured(const DrawModeEnum type,
                                   const View<ColoredTexVert> verts,
                                   const GLRenderState &state)
{
    getFunctions().renderColoredTextured(type, verts, state);
}

void OpenGL::renderPlainFullScreenQuad(const GLRenderState &renderState)
{
    auto &gl = getFunctions();
    gl.renderFullScreenTriangle(gl.getShaderPrograms().getFullScreenShader(),
                                renderState.withDepthFunction(std::nullopt));
}

void OpenGL::cleanup()
{
    getFunctions().cleanup();
}

GLRenderState OpenGL::getDefaultRenderState()
{
    return GLRenderState{};
}

void OpenGL::resetNamedColorsBuffer()
{
    getFunctions().getSharedVbos().reset(Legacy::SharedVboEnum::NamedColorsBlock);
    m_uboManager.invalidate(Legacy::SharedVboEnum::NamedColorsBlock);
}

void OpenGL::initializeRenderer(const float devicePixelRatio)
{
    setDevicePixelRatio(devicePixelRatio);

    auto &funcs = getFunctions();

    // REVISIT: Move this somewhere else?
    GLint maxSamples = 0;
    funcs.glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    OpenGLConfig::setMaxSamples(maxSamples);

    GLint maxUboBindings = 0;
    funcs.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUboBindings);
    assert(static_cast<GLint>(Legacy::NUM_SHARED_VBOS) <= maxUboBindings);

    m_rendererInitialized = true;
}

void OpenGL::renderFont3d(const SharedMMTexture &texture, const View<FontVert3d> verts)
{
    getFunctions().renderFont3d(texture, verts);
}

void OpenGL::initializeOpenGLFunctions()
{
    getFunctions().initializeOpenGLFunctions();
}

const char *OpenGL::glGetString(GLenum name)
{
    return as_cstring(getFunctions().glGetString(name));
}

float OpenGL::getDevicePixelRatio() const
{
    return getFunctions().getDevicePixelRatio();
}

void OpenGL::glViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    getFunctions().glViewport(x, y, w, h);
}

void OpenGL::setDevicePixelRatio(const float devicePixelRatio)
{
    getFunctions().setDevicePixelRatio(devicePixelRatio);
}

// NOTE: Technically we could assert that the SharedMMTexture::getId() == id,
// but this is treated as an opaque pointer and we don't include the header for it.
void OpenGL::setTextureLookup(const MMTextureId id, SharedMMTexture tex)
{
    getFunctions().getTexLookup().set(id, std::move(tex));
}

void OpenGL::uploadArrayLayer(const SharedMMTexture &array, int layer, const View<QImage> images)
{
    auto &gl = getFunctions();
    MMTexture &tex = deref(array);
    QOpenGLTexture &qtex = deref(tex.get());

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, qtex.textureId());

    assert(images.size() <= static_cast<size_t>(qtex.mipLevels()));
    for (size_t level_num = 0; level_num < images.size(); ++level_num) {
        assert(images[level_num].width() == std::max(1, qtex.width() >> level_num));
        assert(images[level_num].height() == std::max(1, qtex.height() >> level_num));

        const QImage &image = images[level_num];
        gl.glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                           static_cast<GLint>(level_num),
                           0,
                           0,
                           layer,
                           image.width(),
                           image.height(),
                           1,
                           GL_RGBA,
                           GL_UNSIGNED_BYTE,
                           image.constBits());
    }

    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void OpenGL::generateMipmaps(const SharedMMTexture &array)
{
    auto &gl = getFunctions();
    MMTexture &tex = deref(array);
    QOpenGLTexture &qtex = deref(tex.get());

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, qtex.textureId());
    gl.glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
