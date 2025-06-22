#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/utils.h"
#include "OpenGLTypes.h"

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <QSurfaceFormat>
#include <qopengl.h>

namespace Legacy {
class Functions;
} // namespace Legacy

class NODISCARD OpenGL final
{
private:
    static std::string g_highest_reportable_version_string;
    std::shared_ptr<Legacy::Functions> m_opengl;
    bool m_rendererInitialized = false;

private:
    NODISCARD auto &getFunctions() { return deref(m_opengl); }
    NODISCARD const auto &getFunctions() const { return deref(m_opengl); }
    NODISCARD const auto &getSharedFunctions() { return m_opengl; }

public:
    OpenGL();
    ~OpenGL();
    OpenGL(const OpenGL &) = delete;
    OpenGL &operator=(const OpenGL &) = delete;

public:
    NODISCARD static QSurfaceFormat createDefaultSurfaceFormat();
    NODISCARD static std::string getHighestReportableVersionString();

public:
    /* must be called before any other functions */
    void initializeOpenGLFunctions();
    void initializeRenderer(float devicePixelRatio);
    NODISCARD const char *glGetString(GLenum name);
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
    NODISCARD bool tryEnableMultisampling(int samples);

public:
    NODISCARD UniqueMesh createPointBatch(const std::vector<ColorVert> &verts);

public:
    // plain means the color is defined by uniform
    NODISCARD UniqueMesh createPlainLineBatch(const std::vector<glm::vec3> &verts);
    // colored means the color is defined by attribute
    NODISCARD UniqueMesh createColoredLineBatch(const std::vector<ColorVert> &verts);

public:
    // plain means the color is defined by uniform
    NODISCARD UniqueMesh createPlainTriBatch(const std::vector<glm::vec3> &verts);
    NODISCARD UniqueMesh createColoredTriBatch(const std::vector<ColorVert> &verts);

public:
    // plain means the color is defined by uniform
    NODISCARD UniqueMesh createPlainQuadBatch(const std::vector<glm::vec3> &verts);
    // colored means the color is defined by attribute
    NODISCARD UniqueMesh createColoredQuadBatch(const std::vector<ColorVert> &verts);
    NODISCARD UniqueMesh createTexturedQuadBatch(const std::vector<TexVert> &verts,
                                                 MMTextureId texture);
    NODISCARD UniqueMesh createColoredTexturedQuadBatch(const std::vector<ColoredTexVert> &verts,
                                                        MMTextureId texture);

    NODISCARD UniqueMesh createFontMesh(const SharedMMTexture &texture,
                                        DrawModeEnum mode,
                                        const std::vector<FontVert3d> &batch);

protected:
    void renderPlain(DrawModeEnum type,
                     const std::vector<glm::vec3> &verts,
                     const GLRenderState &state);
    void renderColored(DrawModeEnum type,
                       const std::vector<ColorVert> &verts,
                       const GLRenderState &state);
    void renderTextured(DrawModeEnum type,
                        const std::vector<TexVert> &verts,
                        const GLRenderState &state);
    void renderColoredTextured(DrawModeEnum type,
                               const std::vector<ColoredTexVert> &verts,
                               const GLRenderState &state);

public:
    void renderPoints(const std::vector<ColorVert> &verts, const GLRenderState &state);

public:
    void renderPlainLines(const std::vector<glm::vec3> &verts, const GLRenderState &state)
    {
        renderPlain(DrawModeEnum::LINES, verts, state);
    }
    void renderPlainTris(const std::vector<glm::vec3> &verts, const GLRenderState &state)
    {
        renderPlain(DrawModeEnum::TRIANGLES, verts, state);
    }
    void renderPlainQuads(const std::vector<glm::vec3> &verts, const GLRenderState &state)
    {
        renderPlain(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderColoredLines(const std::vector<ColorVert> &verts, const GLRenderState &state)
    {
        renderColored(DrawModeEnum::LINES, verts, state);
    }
    void renderColoredTris(const std::vector<ColorVert> &verts, const GLRenderState &state)
    {
        renderColored(DrawModeEnum::TRIANGLES, verts, state);
    }
    void renderColoredQuads(const std::vector<ColorVert> &verts, const GLRenderState &state)
    {
        renderColored(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderTexturedQuads(const std::vector<TexVert> &verts, const GLRenderState &state)
    {
        renderTextured(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderColoredTexturedQuads(const std::vector<ColoredTexVert> &verts,
                                    const GLRenderState &state)
    {
        renderColoredTextured(DrawModeEnum::QUADS, verts, state);
    }

public:
    void renderFont3d(const SharedMMTexture &texture, const std::vector<FontVert3d> &verts);

public:
    void clear(const Color &color);
    void clearDepth();
    void renderPlainFullScreenQuad(const GLRenderState &state);

public:
    void cleanup();
    void setTextureLookup(MMTextureId, SharedMMTexture);
};
