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
        m_opengl = Legacy::Functions::alloc<Legacy::FunctionsGL33>();
        break;
    case OpenGLProber::BackendType::GLES:
        m_opengl = Legacy::Functions::alloc<Legacy::FunctionsES30>();
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

UniqueMesh OpenGL::createPointBatch(const std::vector<ColorVert> &batch)
{
    return getFunctions().createPointBatch(batch);
}

UniqueMesh OpenGL::createPlainLineBatch(const std::vector<glm::vec3> &batch)
{
    return getFunctions().createPlainBatch(DrawModeEnum::LINES, batch);
}

UniqueMesh OpenGL::createColoredLineBatch(const std::vector<ColorVert> &batch)
{
    return getFunctions().createColoredBatch(DrawModeEnum::LINES, batch);
}

UniqueMesh OpenGL::createPlainTriBatch(const std::vector<glm::vec3> &batch)
{
    return getFunctions().createPlainBatch(DrawModeEnum::TRIANGLES, batch);
}

UniqueMesh OpenGL::createColoredTriBatch(const std::vector<ColorVert> &batch)
{
    return getFunctions().createColoredBatch(DrawModeEnum::TRIANGLES, batch);
}

UniqueMesh OpenGL::createPlainQuadBatch(const std::vector<glm::vec3> &batch)
{
    return getFunctions().createPlainBatch(DrawModeEnum::QUADS, batch);
}

UniqueMesh OpenGL::createColoredQuadBatch(const std::vector<ColorVert> &batch)
{
    return getFunctions().createColoredBatch(DrawModeEnum::QUADS, batch);
}

UniqueMesh OpenGL::createTexturedQuadBatch(const std::vector<TexVert> &batch,
                                           const MMTextureId texture)
{
    return getFunctions().createTexturedBatch(DrawModeEnum::QUADS, batch, texture);
}

UniqueMesh OpenGL::createColoredTexturedQuadBatch(const std::vector<ColoredTexVert> &batch,
                                                  const MMTextureId texture)
{
    return getFunctions().createColoredTexturedBatch(DrawModeEnum::QUADS, batch, texture);
}

UniqueMesh OpenGL::createRoomQuadTexBatch(const std::vector<RoomQuadTexVert> &batch,
                                          const MMTextureId texture)
{
    return getFunctions().createRoomQuadTexBatch(batch, texture);
}

UniqueMesh OpenGL::createFontMesh(const SharedMMTexture &texture,
                                  const DrawModeEnum mode,
                                  const std::vector<FontVert3d> &batch)
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
                         const std::vector<glm::vec3> &verts,
                         const GLRenderState &state)
{
    getFunctions().renderPlain(type, verts, state);
}

void OpenGL::renderColored(const DrawModeEnum type,
                           const std::vector<ColorVert> &verts,
                           const GLRenderState &state)
{
    getFunctions().renderColored(type, verts, state);
}

void OpenGL::renderPoints(const std::vector<ColorVert> &verts, const GLRenderState &state)
{
    getFunctions().renderPoints(verts, state);
}

void OpenGL::renderTextured(const DrawModeEnum type,
                            const std::vector<TexVert> &verts,
                            const GLRenderState &state)
{
    getFunctions().renderTextured(type, verts, state);
}

void OpenGL::renderColoredTextured(const DrawModeEnum type,
                                   const std::vector<ColoredTexVert> &verts,
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

void OpenGL::bindNamedColorsBuffer()
{
    auto &gl = getFunctions();
    const auto buffer = Legacy::SharedVboEnum::NamedColorsBlock;
    const auto shared = gl.getSharedVbos().get(buffer);
    Legacy::VBO &vbo = deref(shared);
    if (!vbo) {
        vbo.emplace(gl.shared_from_this());
        // the shader is declared vec4, so the data has to be 4 floats per entry.
        const auto &vec4_colors = XNamedColor::getAllColorsAsVec4();
        std::ignore = gl.setUbo(vbo.get(), vec4_colors, BufferUsageEnum::DYNAMIC_DRAW);
    }
    gl.glBindBufferBase(GL_UNIFORM_BUFFER, buffer, vbo.get());
}

void OpenGL::resetNamedColorsBuffer()
{
    getFunctions().getSharedVbos().reset(Legacy::SharedVboEnum::NamedColorsBlock);
}

void OpenGL::initializeRenderer(const float devicePixelRatio)
{
    setDevicePixelRatio(devicePixelRatio);

    // REVISIT: Move this somewhere else?
    GLint maxSamples = 0;
    getFunctions().glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    OpenGLConfig::setMaxSamples(maxSamples);

    m_rendererInitialized = true;
}

void OpenGL::renderFont3d(const SharedMMTexture &texture, const std::vector<FontVert3d> &verts)
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

void OpenGL::initArrayFromFiles(const SharedMMTexture &array, const std::vector<QString> &input)
{
    auto &gl = getFunctions();
    MMTexture &tex = deref(array);
    QOpenGLTexture &qtex = deref(tex.get());

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, qtex.textureId());

    const auto numLayers = static_cast<GLsizei>(input.size());
    for (GLsizei i = 0; i < numLayers; ++i) {
        const QImage image = QImage{input[static_cast<size_t>(i)]}.mirrored().convertToFormat(
            QImage::Format_RGBA8888);
        if (image.width() != qtex.width() || image.height() != qtex.height()) {
            std::ostringstream oss;
            oss << "Image is " << image.width() << "x" << image.height() << ", but expected "
                << qtex.width() << "x" << qtex.height();
            auto s = std::move(oss).str();
            MMLOG_ERROR() << s.c_str();
        }
        gl.glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                           0,
                           0,
                           0,
                           i,
                           image.width(),
                           image.height(),
                           1,
                           GL_RGBA,
                           GL_UNSIGNED_BYTE,
                           image.constBits());
    }

    gl.glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void OpenGL::initArrayFromImages(const SharedMMTexture &array,
                                 const std::vector<std::vector<QImage>> &input)
{
    auto &gl = getFunctions();
    MMTexture &tex = deref(array);
    QOpenGLTexture &qtex = deref(tex.get());

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, qtex.textureId());

    const auto numImages = input.size();
    for (size_t z = 0; z < numImages; ++z) {
        const auto &layer = input[z];
        const auto numLevels = layer.size();
        assert(numLevels > 0);
        const auto ipow2 = 1 << (numLevels - 1);
        assert(ipow2 == layer.front().width());
        assert(ipow2 == layer.front().height());

        for (size_t level_num = 0; level_num < numLevels; ++level_num) {
            const QImage image = layer[level_num].convertToFormat(QImage::Format_RGBA8888);
            if (image.width() != (qtex.width() >> level_num)
                || image.height() != (qtex.height() >> level_num)) {
                std::ostringstream oss;
                oss << "Image is " << image.width() << "x" << image.height() << ", but expected "
                    << (qtex.width() >> level_num) << "x" << (qtex.height() >> level_num)
                    << " for level " << level_num;
                auto s = std::move(oss).str();
                MMLOG_ERROR() << s.c_str();
            }

            gl.glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                               static_cast<GLint>(level_num),
                               0,
                               0,
                               static_cast<GLint>(z),
                               image.width(),
                               image.height(),
                               1,
                               GL_RGBA,
                               GL_UNSIGNED_BYTE,
                               image.constBits());
        }
    }

    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
