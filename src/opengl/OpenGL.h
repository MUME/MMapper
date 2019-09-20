#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <qopengl.h>

#include "../global/utils.h"
#include "OpenGLTypes.h"

namespace Legacy {
class Functions;
} // namespace Legacy

class OpenGL final
{
private:
    std::shared_ptr<Legacy::Functions> m_opengl;
    bool m_rendererInitialized = false;

private:
    auto &getFunctions() { return deref(m_opengl); }
    const auto &getFunctions() const { return deref(m_opengl); }
    const auto &getSharedFunctions() { return m_opengl; }

public:
    OpenGL();
    ~OpenGL();

public:
    /* must be called before any other functions */
    void initializeOpenGLFunctions();
    void initializeRenderer(float devicePixelRatio);
    const char *glGetString(GLenum name);
    float getDevicePixelRatio() const;
    bool isRendererInitialized() const { return m_rendererInitialized; }

public:
    glm::mat4 getProjectionMatrix() const;
    Viewport getViewport() const;
    Viewport getPhysicalViewport() const;
    void setProjectionMatrix(const glm::mat4 &m);
    void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);

public:
    bool tryEnableMultisampling(int samples);

public:
    UniqueMesh createPointBatch(const std::vector<ColorVert> &verts);

public:
    // plain means the color is defined by uniform
    UniqueMesh createPlainLineBatch(const std::vector<glm::vec3> &verts);
    // colored means the color is defined by attribute
    UniqueMesh createColoredLineBatch(const std::vector<ColorVert> &verts);

public:
    // plain means the color is defined by uniform
    UniqueMesh createPlainTriBatch(const std::vector<glm::vec3> &verts);
    UniqueMesh createColoredTriBatch(const std::vector<ColorVert> &verts);

public:
    // plain means the color is defined by uniform
    UniqueMesh createPlainQuadBatch(const std::vector<glm::vec3> &verts);
    // colored means the color is defined by attribute
    UniqueMesh createColoredQuadBatch(const std::vector<ColorVert> &verts);
    UniqueMesh createTexturedQuadBatch(const std::vector<TexVert> &verts,
                                       const SharedMMTexture &texture);
    UniqueMesh createColoredTexturedQuadBatch(const std::vector<ColoredTexVert> &verts,
                                              const SharedMMTexture &texture);

    UniqueMesh createFontMesh(const SharedMMTexture &texture,
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
};
