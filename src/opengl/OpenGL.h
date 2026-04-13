#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Badge.h"
#include "../global/View.h"
#include "../global/utils.h"
#include "OpenGLTypes.h"
#include "UboManager.h"

#include <memory>

#include <glm/glm.hpp>

#include <QImage>
#include <QString>
#include <QSurfaceFormat>
#include <qopengl.h>

class MapCanvas;
class GLWeather;
namespace Legacy {
class Functions;
} // namespace Legacy

class NODISCARD OpenGL final
{
private:
    std::shared_ptr<Legacy::Functions> m_opengl;
    Legacy::UboManager m_uboManager;
    bool m_rendererInitialized = false;

private:
    NODISCARD auto &getFunctions() { return deref(m_opengl); }
    NODISCARD const auto &getFunctions() const { return deref(m_opengl); }
    NODISCARD const auto &getSharedFunctions() { return m_opengl; }

public:
    explicit OpenGL();
    ~OpenGL();
    OpenGL(const OpenGL &) = delete;
    OpenGL &operator=(const OpenGL &) = delete;

public:
    NODISCARD const auto &getSharedFunctions(Badge<MapCanvas>) { return getSharedFunctions(); }
    NODISCARD const auto &getSharedFunctions(Badge<GLWeather>) { return getSharedFunctions(); }

public:
    /* must be called before any other functions */
    void initializeOpenGLFunctions();
    void initializeRenderer(float devicePixelRatio);
    NODISCARD const char *glGetString(GLenum name);
    NODISCARD int glGetInteger(GLenum name);
    void setDevicePixelRatio(float devicePixelRatio);
    NODISCARD float getDevicePixelRatio() const;
    NODISCARD bool isRendererInitialized() const { return m_rendererInitialized; }

public:
    NODISCARD glm::mat4 getProjectionMatrix() const;
    NODISCARD Viewport getViewport() const;
    NODISCARD Viewport getPhysicalViewport() const;
    void setProjectionMatrix(const glm::mat4 &m);
    void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);

public:
    void configureFbo(int samples);
    void bindFbo();
    void releaseFbo();
    void blitFboToDefault();

public:
    NODISCARD UniqueMesh createPointBatch(View<ColorVert> verts);

public:
    // plain means the color is defined by uniform
    NODISCARD UniqueMesh createPlainLineBatch(View<glm::vec3> verts);
    // colored means the color is defined by attribute
    NODISCARD UniqueMesh createColoredLineBatch(View<ColorVert> verts);

public:
    // plain means the color is defined by uniform
    NODISCARD UniqueMesh createPlainTriBatch(View<glm::vec3> verts);
    NODISCARD UniqueMesh createColoredTriBatch(View<ColorVert> verts);

public:
    // plain means the color is defined by uniform
    NODISCARD UniqueMesh createPlainQuadBatch(View<glm::vec3> verts);
    // colored means the color is defined by attribute
    NODISCARD UniqueMesh createColoredQuadBatch(View<ColorVert> verts);
    NODISCARD UniqueMesh createTexturedQuadBatch(View<TexVert> verts, MMTextureId texture);
    NODISCARD UniqueMesh createColoredTexturedQuadBatch(View<ColoredTexVert> verts,
                                                        MMTextureId texture);

public:
    NODISCARD UniqueMesh createRoomQuadTexBatch(View<RoomQuadTexVert> verts, MMTextureId texture);

public:
    NODISCARD UniqueMesh createFontMesh(const SharedMMTexture &texture,
                                        DrawModeEnum mode,
                                        View<FontVert3d> batch);

protected:
    void renderPlain(DrawModeEnum type, View<glm::vec3> verts, const GLRenderState &state);
    void renderColored(DrawModeEnum type, View<ColorVert> verts, const GLRenderState &state);
    void renderTextured(DrawModeEnum type, View<TexVert> verts, const GLRenderState &state);
    void renderColoredTextured(DrawModeEnum type,
                               View<ColoredTexVert> verts,
                               const GLRenderState &state);

public:
    void renderPoints(View<ColorVert> verts, const GLRenderState &state);

public:
    void renderPlainLines(View<glm::vec3> verts, const GLRenderState &state)
    {
        renderPlain(DrawModeEnum::LINES, verts, state);
    }
    void renderPlainTris(View<glm::vec3> verts, const GLRenderState &state)
    {
        renderPlain(DrawModeEnum::TRIANGLES, verts, state);
    }
    void renderPlainQuads(View<glm::vec3> verts, const GLRenderState &state)
    {
        renderPlain(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderColoredLines(View<ColorVert> verts, const GLRenderState &state)
    {
        renderColored(DrawModeEnum::LINES, verts, state);
    }
    void renderColoredTris(View<ColorVert> verts, const GLRenderState &state)
    {
        renderColored(DrawModeEnum::TRIANGLES, verts, state);
    }
    void renderColoredQuads(View<ColorVert> verts, const GLRenderState &state)
    {
        renderColored(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderTexturedQuads(View<TexVert> verts, const GLRenderState &state)
    {
        renderTextured(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderColoredTexturedQuads(View<ColoredTexVert> verts, const GLRenderState &state)
    {
        renderColoredTextured(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderFont3d(const SharedMMTexture &texture, View<FontVert3d> verts);

public:
    void clear(Color color);
    void clearDepth();
    void renderPlainFullScreenQuad(const GLRenderState &state);

public:
    void cleanup();
    NODISCARD GLRenderState getDefaultRenderState();
    void resetNamedColorsBuffer();
    void setTextureLookup(MMTextureId, SharedMMTexture);

    NODISCARD Legacy::UboManager &getUboManager() { return m_uboManager; }

public:
    void uploadArrayLayer(const SharedMMTexture &array, int layer, View<QImage> images);
    void generateMipmaps(const SharedMMTexture &array);
};
