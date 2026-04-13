#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/Badge.h"
#include "../../global/RuleOf5.h"
#include "../../global/View.h"
#include "../../global/utils.h"
#include "../OpenGLConfig.h"
#include "../OpenGLTypes.h"
#include "../UboBlocks.h"
#include "FBO.h"

#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLExtraFunctions>

class OpenGL;

namespace Legacy {

class Program;
class SharedTfos;
class SharedVaos;
class SharedVbos;
class StaticVaos;
class StaticVbos;
class UboManager;
class VAO;
class VBO;
struct AbstractShaderProgram;
struct PointSizeBinder;
struct ShaderPrograms;

static_assert(Legacy::NUM_SHARED_VBOS > 0, "At least one shared VBO must be defined");
static_assert(static_cast<size_t>(Legacy::SharedVboEnum::NUM_BLOCKS) == Legacy::NUM_SHARED_VBOS,
              "SharedVboEnum must be 0-based and contiguous");

/**
 * @brief Returns the UBO binding index for a shared VBO block.
 *
 * Note: SharedVboEnum values must be 0-based and contiguous.
 */
NODISCARD static inline GLuint getUboBindingIndex(const SharedVboEnum block)
{
    static_assert(static_cast<uint8_t>(SharedVboEnum::NamedColorsBlock) == 0,
                  "SharedVboEnum must be 0-based for UBO binding indices");
    return static_cast<GLuint>(block);
}

#define XFOREACH_SHARED_VAO(X) X(EmptyVao)

#define X_COUNT_VAO(element) +1
static constexpr size_t NUM_SHARED_VAOS = 0 XFOREACH_SHARED_VAO(X_COUNT_VAO);
#undef X_COUNT_VAO

enum class SharedVaoEnum : uint8_t {
#define X_ENUM(element) element,
    XFOREACH_SHARED_VAO(X_ENUM)
#undef X_ENUM
        NUM_VAOS
};

static_assert(static_cast<size_t>(SharedVaoEnum::NUM_VAOS) == NUM_SHARED_VAOS,
              "SharedVaoEnum must be 0-based and contiguous");

NODISCARD static inline GLenum toGLenum(const BufferUsageEnum usage)
{
    switch (usage) {
    case BufferUsageEnum::DYNAMIC_DRAW:
        return GL_DYNAMIC_DRAW;
    case BufferUsageEnum::STATIC_DRAW:
        return GL_STATIC_DRAW;
    case BufferUsageEnum::STREAM_DRAW:
        return GL_STREAM_DRAW;
    }
    std::abort();
}

// REVISIT: Find this a new home when there's more than one OpenGL implementation.
// Note: This version is only suitable for drawArrays(). You'll need another function
// to transform indices if you want to use it with drawElements().
template<typename VertexType_>
NODISCARD static inline std::vector<VertexType_> convertQuadsToTris(const View<VertexType_> quads)
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
    NODISCARD static std::shared_ptr<Functions> alloc(UboManager &uboManager)
    {
        return std::make_shared<T>(Badge<Functions>{}, uboManager);
    }

private:
    using Base = QOpenGLExtraFunctions;
    glm::mat4 m_viewProj = glm::mat4(1);
    Viewport m_viewport;
    float m_devicePixelRatio = 1.f;
    std::unique_ptr<ShaderPrograms> m_shaderPrograms;
    std::unique_ptr<SharedVaos> m_sharedVaos;
    std::unique_ptr<SharedVbos> m_sharedVbos;
    std::unique_ptr<StaticVaos> m_staticVaos;
    std::unique_ptr<StaticVbos> m_staticVbos;
    std::unique_ptr<TexLookup> m_texLookup;
    std::unique_ptr<FBO> m_fbo;
    UboManager &m_uboManager;

protected:
    explicit Functions(Badge<Functions>, UboManager &uboManager);

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
    using Base::initializeOpenGLFunctions;

public:
    using Base::glActiveTexture;
    using Base::glAttachShader;
    using Base::glBindBuffer;
    using Base::glBindBufferBase;
    using Base::glBindBufferRange;
    using Base::glBlendEquationSeparate;
    using Base::glBlendFunc;
    using Base::glBlendFuncSeparate;
    using Base::glBufferData;
    using Base::glBufferSubData;
    using Base::glClear;
    using Base::glClearColor;
    using Base::glCompileShader;
    using Base::glCreateProgram;
    using Base::glCreateShader;
    using Base::glCullFace;
    using Base::glDeleteBuffers;
    using Base::glDeleteProgram;
    using Base::glDeleteShader;
    using Base::glDeleteTransformFeedbacks;
    using Base::glDeleteVertexArrays;
    using Base::glDepthFunc;
    using Base::glDetachShader;
    using Base::glDisable;
    using Base::glDisableVertexAttribArray;
    using Base::glDrawArrays;
    using Base::glDrawArraysInstanced;
    using Base::glDrawElementsInstanced;
    using Base::glEnable;
    using Base::glEnableVertexAttribArray;
    using Base::glGenBuffers;
    using Base::glGenerateMipmap;
    using Base::glGenTransformFeedbacks;
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
    using Base::glGetUniformBlockIndex;
    using Base::glGetUniformLocation;
    using Base::glHint;
    using Base::glIsBuffer;
    using Base::glIsProgram;
    using Base::glIsShader;
    using Base::glIsTexture;
    using Base::glLinkProgram;
    using Base::glPixelStorei;
    using Base::glShaderSource;

    using Base::glBeginTransformFeedback;
    using Base::glBindTransformFeedback;
    using Base::glEndTransformFeedback;
    using Base::glTransformFeedbackVaryings;

    using Base::glTexSubImage3D;
    using Base::glUniform1fv;
    using Base::glUniform1iv;
    using Base::glUniform4fv;
    using Base::glUniform4iv;
    using Base::glUniformBlockBinding;

    /**
     * @brief Assigns a fixed binding point to a uniform block in a program.
     * @param program The shader program.
     * @param block The uniform block.
     *
     * Note: This uses the enum value as the fixed binding point.
     */
    void glUniformBlockBinding(GLuint program, SharedVboEnum block);
    void glUniformBlockBinding(const Program &program, SharedVboEnum block);

    /**
     * @brief Automatically assigns fixed binding points to all known uniform blocks.
     * @param program The shader program after linking.
     */
    void applyDefaultUniformBlockBindings(GLuint program);
    using Base::glUniformMatrix4fv;
    using Base::glUseProgram;
    using Base::glVertexAttribDivisor;
    using Base::glVertexAttribPointer;

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
        const auto &offset = m_viewport.offset;
        const auto &size = m_viewport.size;
        return Viewport{{scalei(offset.x), scalei(offset.y)}, {scalei(size.x), scalei(size.y)}};
    }

public:
    NODISCARD glm::mat4 getProjectionMatrix() const { return m_viewProj; }

    void setProjectionMatrix(const glm::mat4 &viewProj) { m_viewProj = viewProj; }

public:
    void cleanup();

    NODISCARD ShaderPrograms &getShaderPrograms();

    NODISCARD SharedVaos &getSharedVaos();
    NODISCARD SharedVbos &getSharedVbos();
    NODISCARD StaticVaos &getStaticVaos();
    NODISCARD StaticVbos &getStaticVbos();

    NODISCARD TexLookup &getTexLookup();
    NODISCARD FBO &getFBO();
    NODISCARD UboManager &getUboManager();

private:
    friend PointSizeBinder;
    /// platform-specific (ES vs GL)
    void enableProgramPointSize(bool enable) { virt_enableProgramPointSize(enable); }

public:
    /// platform-specific (ES vs GL)
    NODISCARD const char *getShaderVersion() const { return virt_getShaderVersion(); }

    NODISCARD virtual std::optional<GLenum> virt_toGLenum(DrawModeEnum mode) = 0;
    virtual void virt_enableProgramPointSize(bool enable) = 0;
    NODISCARD virtual const char *virt_getShaderVersion() const = 0;
    virtual void virt_glUniformBlockBinding(GLuint program, SharedVboEnum block);

public:
    NODISCARD static const char *getUniformBlockName(SharedVboEnum block);

    /// platform-specific (ES vs GL)
    NODISCARD std::optional<GLenum> toGLenum(const DrawModeEnum mode)
    {
        return virt_toGLenum(mode);
    }

public:
    void enableAttrib(
        GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, size_t offset);
    void enableAttribI(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset);

public:
    NODISCARD UniqueMesh createPointBatch(View<ColorVert> batch);
    NODISCARD UniqueMesh createPlainLineBatch(View<glm::vec3> batch);
    NODISCARD UniqueMesh createColoredLineBatch(View<ColorVert> batch);

public:
    NODISCARD UniqueMesh createPlainBatch(DrawModeEnum mode, View<glm::vec3> batch);
    NODISCARD UniqueMesh createColoredBatch(DrawModeEnum mode, View<ColorVert> batch);
    NODISCARD UniqueMesh createTexturedBatch(DrawModeEnum mode,
                                             View<TexVert> batch,
                                             MMTextureId texture);
    NODISCARD UniqueMesh createColoredTexturedBatch(DrawModeEnum mode,
                                                    View<ColoredTexVert> batch,
                                                    MMTextureId texture);

public:
    NODISCARD UniqueMesh createRoomQuadTexBatch(View<RoomQuadTexVert> batch, MMTextureId texture);

public:
    NODISCARD UniqueMesh createFontMesh(const SharedMMTexture &texture,
                                        DrawModeEnum mode,
                                        View<FontVert3d> batch);

public:
    void renderPoints(View<ColorVert> verts, const GLRenderState &state);
    void renderPlainLines(View<glm::vec3> verts, const GLRenderState &state);
    void renderColoredLines(View<ColorVert> verts, const GLRenderState &state);

    void renderPlain(DrawModeEnum mode, View<glm::vec3> verts, const GLRenderState &state);
    void renderColored(DrawModeEnum mode, View<ColorVert> verts, const GLRenderState &state);
    void renderTextured(DrawModeEnum mode, View<TexVert> verts, const GLRenderState &state);
    void renderColoredTextured(DrawModeEnum mode,
                               View<ColoredTexVert> verts,
                               const GLRenderState &state);
    void renderFont3d(const SharedMMTexture &texture, View<FontVert3d> verts);

public:
    void renderFullScreenTriangle(const std::shared_ptr<AbstractShaderProgram> &prog_ptr,
                                  const GLRenderState &state);

public:
    void checkError();

public:
    void configureFbo(int samples);
    void bindFbo();
    void releaseFbo();
    void blitFboToDefault();

private:
    void bindActiveTexture_internal(const GLenum activeTexture,
                                    const GLenum target,
                                    const GLuint texture)
    {
        Base &base = *this;
        base.glActiveTexture(activeTexture);
        base.glBindTexture(target, texture);
    }

public:
    NODISCARD auto bindTexture(const GLenum activeTexture, const GLenum target, const GLuint texture)
    {
        struct NODISCARD Unbinder final
        {
        private:
            Functions &m_self;
            const GLenum m_activeTexture;
            const GLenum m_target;

        public:
            explicit Unbinder(Functions &self, const GLenum activeTexture_, const GLenum target_)
                : m_self{self}
                , m_activeTexture{activeTexture_}
                , m_target{target_}
            {}
            ~Unbinder() { m_self.bindActiveTexture_internal(m_activeTexture, m_target, 0); }
            Unbinder(const Unbinder &) = delete;
            Unbinder &operator=(const Unbinder &) = delete;
        };
        bindActiveTexture_internal(activeTexture, target, texture);
        return Unbinder{*this, activeTexture, target};
    }

    NODISCARD auto bindTexture0(const GLenum target, const GLuint texture)
    {
        return bindTexture(GL_TEXTURE0, target, texture);
    }

public:
    void glBindVertexArray(Badge<VAO>, const GLuint array) { Base::glBindVertexArray(array); }
};
} // namespace Legacy

DEFINE_ENUM_COUNT(Legacy::SharedVboEnum, Legacy::NUM_SHARED_VBOS)
