#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/Badge.h"
#include "../../global/RuleOf5.h"
#include "../../global/utils.h"
#include "../OpenGLConfig.h"
#include "../OpenGLTypes.h"
#include "FBO.h"

#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLExtraFunctions>

class OpenGL;

namespace Legacy {

class StaticVbos;
struct ShaderPrograms;
struct PointSizeBinder;

NODISCARD static inline GLenum toGLenum(const BufferUsageEnum usage)
{
    switch (usage) {
    case BufferUsageEnum::DYNAMIC_DRAW:
        return GL_DYNAMIC_DRAW;
    case BufferUsageEnum::STATIC_DRAW:
        return GL_STATIC_DRAW;
    }
    std::abort();
}

// REVISIT: Find this a new home when there's more than one OpenGL implementation.
// Note: This version is only suitable for drawArrays(). You'll need another function
// to transform indices if you want to use it with drawElements().
template<typename VertexType_>
NODISCARD static inline std::vector<VertexType_> convertQuadsToTris(
    const std::vector<VertexType_> &quads)
{
    // d-c
    // |/|
    // a-b
    const static constexpr int TRIANGLE_VERTS_PER_QUAD = 6;
    const size_t numQuads = quads.size() / VERTS_PER_QUAD;
    const size_t expected = numQuads * TRIANGLE_VERTS_PER_QUAD;
    std::vector<VertexType_> triangles;
    triangles.reserve(expected);
    const auto *it = quads.data();
    for (size_t i = 0; i < numQuads; ++i) {
        const auto &a = *(it++);
        const auto &b = *(it++);
        const auto &c = *(it++);
        const auto &d = *(it++);
        triangles.emplace_back(a);
        triangles.emplace_back(b);
        triangles.emplace_back(c);
        triangles.emplace_back(c);
        triangles.emplace_back(d);
        triangles.emplace_back(a);
    }
    assert(triangles.size() == expected);
    return triangles;
}

class Functions;
class FunctionsGL33;
class FunctionsES30;
using SharedFunctions = std::shared_ptr<Functions>;
using WeakFunctions = std::weak_ptr<Functions>;

/// \c Legacy::Functions implements both GL 3.X and ES 3.X (based on a subset of
/// ES 3.X)
class NODISCARD Functions : protected QOpenGLExtraFunctions,
                            public std::enable_shared_from_this<Functions>
{
    friend class FunctionsGL33;
    friend class FunctionsES30;

public:
    template<typename T>
    NODISCARD static std::shared_ptr<Functions> alloc()
    {
        return std::make_shared<T>(Badge<Functions>{});
    }

private:
    using Base = QOpenGLExtraFunctions;
    glm::mat4 m_viewProj = glm::mat4(1);
    Viewport m_viewport;
    float m_devicePixelRatio = 1.f;
    std::unique_ptr<ShaderPrograms> m_shaderPrograms;
    std::unique_ptr<StaticVbos> m_staticVbos;
    std::unique_ptr<TexLookup> m_texLookup;
    std::unique_ptr<FBO> m_fbo;
    std::vector<std::shared_ptr<IRenderable>> m_staticMeshes;

protected:
    explicit Functions(Badge<Functions>);

public:
    virtual ~Functions();
    DELETE_CTORS_AND_ASSIGN_OPS(Functions);

public:
    NODISCARD float getDevicePixelRatio() const { return m_devicePixelRatio; }

    void setDevicePixelRatio(const float devicePixelRatio)
    {
        constexpr float ratio = 64.f;
        constexpr float inv_ratio = 1.f / ratio;
        if (!std::isfinite(devicePixelRatio) || !isClamped(devicePixelRatio, inv_ratio, ratio)) {
            throw std::invalid_argument("devicePixelRatio");
        }
        m_devicePixelRatio = devicePixelRatio;
    }

public:
    // The purpose of this function is to safely manage the lifetime of reused meshes
    // like the full screen quad mesh. Caller is expected to only keep a weak pointer
    // to the mesh. See OpenGL::renderPlainFullScreenQuad().
    void addSharedMesh(Badge<OpenGL>, std::shared_ptr<IRenderable> mesh)
    {
        m_staticMeshes.emplace_back(std::move(mesh));
    }

public:
    using Base::initializeOpenGLFunctions;

public:
    using Base::glActiveTexture;
    using Base::glAttachShader;
    using Base::glBindBuffer;
    using Base::glBindFramebuffer;
    using Base::glBindTexture;
    using Base::glBindVertexArray;
    using Base::glBlendFunc;
    using Base::glBlendFuncSeparate;
    using Base::glBlitFramebuffer;
    using Base::glBufferData;
    using Base::glClear;
    using Base::glClearColor;
    using Base::glColorMask;
    using Base::glCompileShader;
    using Base::glCreateProgram;
    using Base::glCreateShader;
    using Base::glCullFace;
    using Base::glDeleteBuffers;
    using Base::glDeleteProgram;
    using Base::glDeleteShader;
    using Base::glDeleteVertexArrays;
    using Base::glDepthFunc;
    using Base::glDepthMask;
    using Base::glDetachShader;
    using Base::glDisable;
    using Base::glDisableVertexAttribArray;
    using Base::glDrawArrays;
    using Base::glEnable;
    using Base::glEnableVertexAttribArray;
    using Base::glFlush;
    using Base::glGenBuffers;
    using Base::glGenerateMipmap;
    using Base::glGenVertexArrays;
    using Base::glGetAttribLocation;
    using Base::glGetIntegerv;
    using Base::glGetProgramInfoLog;
    using Base::glGetProgramiv;
    using Base::glGetShaderInfoLog;
    using Base::glGetShaderiv;
    using Base::glGetString;
    using Base::glGetTexLevelParameteriv;
    using Base::glGetTexParameteriv;
    using Base::glGetUniformLocation;
    using Base::glFinish;
    using Base::glHint;
    using Base::glIsBuffer;
    using Base::glIsProgram;
    using Base::glIsShader;
    using Base::glIsTexture;
    using Base::glLinkProgram;
    using Base::glPixelStorei;
    using Base::glShaderSource;
    using Base::glStencilMask;
    using Base::glTexSubImage3D;
    using Base::glUniform1fv;
    using Base::glUniform1iv;
    using Base::glUniform4fv;
    using Base::glUniform4iv;
    using Base::glUniformMatrix4fv;
    using Base::glUseProgram;
    using Base::glVertexAttribPointer;

public:
    void glLineWidth(const GLfloat lineWidth)
    {
        // REVISIT: Only width 1 is guaranteed to be supported for core profiles
        if (OpenGLConfig::getIsCompat()) {
            Base::glLineWidth(lineWidth);
        }
    }

public:
    void glViewport(const GLint x, const GLint y, const GLsizei width, const GLsizei height)
    {
        m_viewport = Viewport{{x, y}, {width, height}};
        Base::glViewport(scalei(x), scalei(y), scalei(width), scalei(height));
    }

private:
    NODISCARD float scalef(const float f) const
    {
        static_assert(std::is_same_v<float, GLfloat>);
        return f * m_devicePixelRatio;
    }

    NODISCARD int scalei(const int n) const
    {
        static_assert(std::is_same_v<int, GLint>);
        static_assert(std::is_same_v<int, GLsizei>);
        return static_cast<int>(std::lround(scalef(static_cast<float>(n))));
    }

public:
    NODISCARD Viewport getViewport() const { return m_viewport; }

    NODISCARD Viewport getPhysicalViewport() const
    {
        const glm::ivec2 &offset = m_viewport.offset;
        const glm::ivec2 &size = m_viewport.size;
        return Viewport{{scalei(offset.x), scalei(offset.y)}, {scalei(size.x), scalei(size.y)}};
    }

public:
    NODISCARD glm::mat4 getProjectionMatrix() const { return m_viewProj; }

    void setProjectionMatrix(const glm::mat4 &viewProj) { m_viewProj = viewProj; }

public:
    void cleanup();

    NODISCARD ShaderPrograms &getShaderPrograms();

    NODISCARD StaticVbos &getStaticVbos();

    NODISCARD TexLookup &getTexLookup();

    NODISCARD FBO &getFBO();

private:
    friend PointSizeBinder;
    /// platform-specific (ES vs GL)
    void enableProgramPointSize(bool enable) { virt_enableProgramPointSize(enable); }

public:
    /// platform-specific (ES vs GL)
    NODISCARD const char *getShaderVersion() const { return virt_getShaderVersion(); }

protected:
    NODISCARD virtual bool virt_canRenderQuads() = 0;
    NODISCARD virtual std::optional<GLenum> virt_toGLenum(DrawModeEnum mode) = 0;
    virtual void virt_enableProgramPointSize(bool enable) = 0;
    NODISCARD virtual const char *virt_getShaderVersion() const = 0;

private:
    template<typename VertexType_>
    NODISCARD GLsizei setVbo_internal(const GLuint vbo,
                                      const std::vector<VertexType_> &batch,
                                      const BufferUsageEnum usage)
    {
        const auto numVerts = static_cast<GLsizei>(batch.size());
        const auto vertSize = static_cast<GLsizei>(sizeof(VertexType_));
        const auto numBytes = numVerts * vertSize;
        Base::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        Base::glBufferData(GL_ARRAY_BUFFER, numBytes, batch.data(), Legacy::toGLenum(usage));
        Base::glBindBuffer(GL_ARRAY_BUFFER, 0);
        return numVerts;
    }

public:
    /// platform-specific (ES vs GL)
    NODISCARD bool canRenderQuads() { return virt_canRenderQuads(); }

    /// platform-specific (ES vs GL)
    NODISCARD std::optional<GLenum> toGLenum(DrawModeEnum mode) { return virt_toGLenum(mode); }

public:
    void enableAttrib(const GLuint index,
                      const GLint size,
                      const GLenum type,
                      const GLboolean normalized,
                      const GLsizei stride,
                      const GLvoid *const pointer)
    {
        Base::glEnableVertexAttribArray(index);
        Base::glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    }

    template<typename T>
    NODISCARD std::pair<DrawModeEnum, GLsizei> setVbo(
        const DrawModeEnum mode,
        const GLuint vbo,
        const std::vector<T> &batch,
        const BufferUsageEnum usage = BufferUsageEnum::DYNAMIC_DRAW)
    {
        if (mode == DrawModeEnum::QUADS && !canRenderQuads()) {
            return std::pair(DrawModeEnum::TRIANGLES,
                             setVbo_internal(vbo, convertQuadsToTris(batch), usage));
        }
        return std::pair(mode, setVbo_internal(vbo, batch, usage));
    }

    void clearVbo(const GLuint vbo, const BufferUsageEnum usage = BufferUsageEnum::DYNAMIC_DRAW)
    {
        Base::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        Base::glBufferData(GL_ARRAY_BUFFER, 0, nullptr, Legacy::toGLenum(usage));
        Base::glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

public:
    NODISCARD UniqueMesh createPointBatch(const std::vector<ColorVert> &batch);

public:
    NODISCARD UniqueMesh createPlainBatch(DrawModeEnum mode, const std::vector<glm::vec3> &batch);
    NODISCARD UniqueMesh createColoredBatch(DrawModeEnum mode, const std::vector<ColorVert> &batch);
    NODISCARD UniqueMesh createTexturedBatch(DrawModeEnum mode,
                                             const std::vector<TexVert> &batch,
                                             MMTextureId texture);
    NODISCARD UniqueMesh createColoredTexturedBatch(DrawModeEnum mode,
                                                    const std::vector<ColoredTexVert> &batch,
                                                    MMTextureId texture);

public:
    NODISCARD UniqueMesh createFontMesh(const SharedMMTexture &texture,
                                        DrawModeEnum mode,
                                        const std::vector<FontVert3d> &batch);

public:
    void renderPoints(const std::vector<ColorVert> &verts, const GLRenderState &state);

public:
    void renderPlain(DrawModeEnum mode,
                     const std::vector<glm::vec3> &verts,
                     const GLRenderState &state);
    void renderColored(DrawModeEnum mode,
                       const std::vector<ColorVert> &verts,
                       const GLRenderState &state);
    void renderTextured(DrawModeEnum mode,
                        const std::vector<TexVert> &verts,
                        const GLRenderState &state);
    void renderColoredTextured(DrawModeEnum mode,
                               const std::vector<ColoredTexVert> &verts,
                               const GLRenderState &state);
    void renderFont3d(const SharedMMTexture &texture, const std::vector<FontVert3d> &verts);

public:
    void checkError();

public:
    void configureFbo(int samples);
    void bindFbo(GLuint targetId);
    void releaseFbo();
    NODISCARD bool isMultisampling() const;
    void bindFramebuffer(GLuint targetId);
    void blitFboToTarget(GLuint targetId);
};
} // namespace Legacy
