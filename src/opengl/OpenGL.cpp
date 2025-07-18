// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "OpenGL.h"

#include "../display/Textures.h"
#include "../global/logging.h"
#include "./legacy/Legacy.h"
#include "./legacy/Meshes.h"
#include "./legacy/VBO.h"
#include "OpenGLTypes.h"

#include <cassert>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLContext>
#include <QSurfaceFormat>

#ifdef WIN32
extern "C" {
// Prefer discrete nVidia and AMD GPUs by default on Windows
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static const constexpr auto UNDEFINED_VERSION = "Fallback";

namespace { // anonymous

struct GLVersion
{
    int major = 0;
    int minor = 0;

    NODISCARD bool operator>(const GLVersion &other) const
    {
        return (major > other.major) || (major == other.major && minor > other.minor);
    }

    NODISCARD bool operator<(const GLVersion &other) const
    {
        return (major < other.major) || (major == other.major && minor < other.minor);
    }
};

inline std::ostream &operator<<(std::ostream &os, const GLVersion &version)
{
    os << version.major << "." << version.minor;
    return os;
}

struct GLContextCheckResult
{
    bool valid = false;
    GLVersion version = {0, 0};
    bool isCore = false;
    bool isCompat = false;
    bool isDeprecated = false;
    bool isDebug = false;
};

NODISCARD GLContextCheckResult checkContext(QSurfaceFormat format,
                                            GLVersion version,
                                            QSurfaceFormat::OpenGLContextProfile profile)
{
    GLContextCheckResult result;
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create()) {
        MMLOG_DEBUG() << "[GL Check] context.create() failed for requested " << version
                      << (profile == QSurfaceFormat::CoreProfile ? " Core" : " Compat");
        return result;
    }

    QSurfaceFormat actualFormat = context.format();

    result.isCore = (actualFormat.profile() & QSurfaceFormat::CoreProfile);
    result.isCompat = (actualFormat.profile() & QSurfaceFormat::CompatibilityProfile);
    result.isDeprecated = (actualFormat.options() & QSurfaceFormat::DeprecatedFunctions);
    result.isDebug = (actualFormat.options() & QSurfaceFormat::DebugContext);
    result.version = GLVersion{actualFormat.majorVersion(), actualFormat.minorVersion()};

    context.doneCurrent();

    // Check if the actual OpenGL context meets the minimum requirements
    bool profileOk = false;

    if (version < GLVersion{3, 2}) {
        if (profile == QSurfaceFormat::CoreProfile) {
            // Core profile did not exist before GL 3.2
            profileOk = false;
        } else {
            // Must be compatibility-like
            profileOk = result.isCompat || result.isDeprecated;
        }
    } else {
        // For GL 3.2+
        if (profile == QSurfaceFormat::CoreProfile) {
            profileOk = result.isCore;
        } else if (profile == QSurfaceFormat::CompatibilityProfile) {
            profileOk = result.isCompat && result.isDeprecated;
        }
    }

    // If the profile is ok, we consider the context valid even if the version is lower than requested.
    if (profileOk) {
        result.valid = true;
        MMLOG_DEBUG() << "[GL Probe] GL " << result.version << (result.isCore ? " Core" : " Compat")
                      << " is valid";
    }
    return result;
}

struct OpenGLProbeResult
{
    QSurfaceFormat runningFormat = QSurfaceFormat::defaultFormat();
    std::string highestVersion = UNDEFINED_VERSION;
};

NODISCARD std::string formatGLVersionString(const GLContextCheckResult &result)
{
    std::ostringstream oss;
    oss << "GL" << result.version;
    if (!result.isDeprecated && result.version > GLVersion{3, 1}) {
        oss << "core";
    }
    return oss.str();
}

NODISCARD std::optional<GLContextCheckResult> probeCore(QSurfaceFormat &testFormat,
                                                        std::vector<GLVersion> &coreVersions,
                                                        QSurfaceFormat::FormatOptions optionsCoreOnly)
{
    std::optional<GLContextCheckResult> coreResult = std::nullopt;
    for (auto it = coreVersions.begin(); it != coreVersions.end();) {
        const auto &version = *it;
        testFormat.setVersion(version.major, version.minor);
        testFormat.setProfile(QSurfaceFormat::CoreProfile);
        testFormat.setOptions(optionsCoreOnly);

        GLContextCheckResult contextCheckResult = checkContext(testFormat,
                                                               version,
                                                               QSurfaceFormat::CoreProfile);
        if (contextCheckResult.valid) {
            coreResult = contextCheckResult;
            MMLOG_DEBUG() << "[GL Probe] Found highest supported Core version: "
                          << contextCheckResult.version;
            break;
        } else {
            it = coreVersions.erase(it);
        }
    }
    return coreResult;
}

NODISCARD std::optional<GLContextCheckResult> probeCompat(
    QSurfaceFormat &format,
    std::vector<GLVersion> versions,
    QSurfaceFormat::FormatOptions options,
    std::optional<GLContextCheckResult> coreResult)
{
    std::optional<GLContextCheckResult> compatResult = std::nullopt;

    std::vector<GLVersion> compatVersionsToTest = versions;
    if (coreResult) {
        // Filter compatVersions to only include versions <= coreResult
        compatVersionsToTest.erase(std::remove_if(compatVersionsToTest.begin(),
                                                  compatVersionsToTest.end(),
                                                  [&](const GLVersion &ver) {
                                                      return ver > coreResult->version;
                                                  }),
                                   compatVersionsToTest.end());
    }

    for (const auto &version : compatVersionsToTest) {
        format.setVersion(version.major, version.minor);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setOptions(options);
        GLContextCheckResult contextCheckResult = checkContext(format,
                                                               version,
                                                               QSurfaceFormat::CompatibilityProfile);
        if (contextCheckResult.valid) {
            if (contextCheckResult.valid) {
                compatResult = contextCheckResult;
                MMLOG_DEBUG() << "[GL Probe] Found highest supported Compat version: "
                              << compatResult->version;
                break;
            }
        }
    }
    return compatResult;
}

NODISCARD std::string getHighestGLVersion(std::optional<GLContextCheckResult> coreResult,
                                          std::optional<GLContextCheckResult> compatResult)
{
    std::string highestGLVersion;
    if (coreResult && compatResult) {
        if (coreResult->version > compatResult->version) {
            highestGLVersion = formatGLVersionString(*coreResult);
        } else {
            highestGLVersion = formatGLVersionString(*compatResult);
        }
    } else if (coreResult) {
        highestGLVersion = formatGLVersionString(*coreResult);
    } else if (compatResult) {
        highestGLVersion = formatGLVersionString(*compatResult);
    } else {
        highestGLVersion = UNDEFINED_VERSION;
    }
    return highestGLVersion;
}

NODISCARD QSurfaceFormat getOptimalFormat(std::optional<GLContextCheckResult> result)
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    if (result) {
        if ((false)) {
            // REVISIT: GL_INVALID_ENUM in glEnable(GL_POINT_SMOOTH) on Mesa if we use this
            format.setVersion(result->version.major, result->version.minor);
        } else {
            format.setVersion(3, 3);
        }
        format.setProfile(result->isCore ? QSurfaceFormat::CoreProfile
                                         : QSurfaceFormat::CompatibilityProfile);
        QSurfaceFormat::FormatOptions options;
        if (result->isDebug) {
            options |= QSurfaceFormat::DebugContext;
        }
        if (result->isCompat) {
            options |= QSurfaceFormat::DeprecatedFunctions;
        }
        MMLOG_INFO() << "[GL Probe] Optimal running format determined: GL " << format.majorVersion()
                     << "." << format.minorVersion()
                     << " Profile: " << (result->isCore ? "Core" : "Compat")
                     << (result->isDebug ? " (Debug)" : " (NO Debug)");
    } else {
        // Fallback for optimal running format if no context was found at all
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setOptions(QSurfaceFormat::DebugContext);
        MMLOG_ERROR() << "[GL Probe] No suitable GL context found for running format.";
    }
    return format;
}

NODISCARD OpenGLProbeResult probeOpenGLFormats()
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);

    QSurfaceFormat::FormatOptions optionsCompat = QSurfaceFormat::DebugContext
                                                  | QSurfaceFormat::DeprecatedFunctions;
    QSurfaceFormat::FormatOptions optionsCore = QSurfaceFormat::DebugContext;

    // Define lists of versions to try.
    std::vector<GLVersion> coreVersions
        = {{4, 6}, {4, 5}, {4, 4}, {4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3}, {3, 2}};

    std::optional<GLContextCheckResult> coreResult = probeCore(format, coreVersions, optionsCore);
    std::optional<GLContextCheckResult> compatResult = probeCompat(format,
                                                                   coreVersions,
                                                                   optionsCompat,
                                                                   coreResult);

    OpenGLProbeResult result;
    result.highestVersion = getHighestGLVersion(coreResult, compatResult);
    result.runningFormat = getOptimalFormat(compatResult);
    return result;
}

} // namespace

std::string OpenGL::g_highest_reportable_version_string = UNDEFINED_VERSION;

QSurfaceFormat OpenGL::createDefaultSurfaceFormat()
{
    OpenGLProbeResult probeResult = probeOpenGLFormats();
    OpenGL::g_highest_reportable_version_string = probeResult.highestVersion;
    return probeResult.runningFormat;
}

std::string OpenGL::getHighestReportableVersionString()
{
    return OpenGL::g_highest_reportable_version_string;
}

OpenGL::OpenGL()
    : m_opengl{Legacy::Functions::alloc()}
{}

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

bool OpenGL::tryEnableMultisampling(const int requestedSamples)
{
    return getFunctions().tryEnableMultisampling(requestedSamples);
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

UniqueMesh OpenGL::createFontMesh(const SharedMMTexture &texture,
                                  const DrawModeEnum mode,
                                  const std::vector<FontVert3d> &batch)
{
    return getFunctions().createFontMesh(texture, mode, batch);
}

void OpenGL::clear(const Color &color)
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
    using MeshType = Legacy::PlainMesh<glm::vec3>;
    static std::weak_ptr<MeshType> g_mesh;
    auto sharedMesh = g_mesh.lock();
    if (sharedMesh == nullptr) {
        if (IS_DEBUG_BUILD) {
            qDebug() << "allocating shared mesh for renderPlainFullScreenQuad";
        }
        auto &sharedFuncs = getSharedFunctions();
        auto &funcs = deref(sharedFuncs);
        g_mesh = sharedMesh
            = std::make_shared<MeshType>(sharedFuncs,
                                         funcs.getShaderPrograms().getPlainUColorShader());
        funcs.addSharedMesh(Badge<OpenGL>{}, sharedMesh);
        MeshType &mesh = deref(sharedMesh);

        // screen is [-1,+1]^3.
        const auto fullScreenQuad = std::vector{glm::vec3{-1, -1, 0},
                                                glm::vec3{+1, -1, 0},
                                                glm::vec3{+1, +1, 0},
                                                glm::vec3{-1, +1, 0}};
        mesh.setStatic(DrawModeEnum::QUADS, fullScreenQuad);
    }

    MeshType &mesh = deref(sharedMesh);
    const auto oldProj = getProjectionMatrix();
    setProjectionMatrix(glm::mat4(1));
    mesh.render(renderState.withDepthFunction(std::nullopt));
    setProjectionMatrix(oldProj);
}

void OpenGL::cleanup()
{
    getFunctions().cleanup();
}

void OpenGL::initializeRenderer(const float devicePixelRatio)
{
    setDevicePixelRatio(devicePixelRatio);
    m_rendererInitialized = true;
}

void OpenGL::renderFont3d(const SharedMMTexture &texture, const std::vector<FontVert3d> &verts)
{
    getFunctions().renderFont3d(texture, verts);
}

void OpenGL::initializeOpenGLFunctions()
{
    if (!getFunctions().initializeOpenGLFunctions()) {
        throw std::runtime_error("failed to initialize");
    }
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

namespace init_array_helper {

NODISCARD static GLint xglGetTexParameteri(QOpenGLFunctions_3_3_Core &gl,
                                           GLenum target,
                                           GLenum pname)
{
    GLint result = 0;
    gl.glGetTexParameteriv(target, pname, &result);
    return result;
}

NODISCARD static GLint xglGetTexLevelParameteri(QOpenGLFunctions_3_3_Core &gl,
                                                GLenum target,
                                                GLint level,
                                                GLenum pname)
{
    GLint result = 0;
    gl.glGetTexLevelParameteriv(target, level, pname, &result);
    return result;
}

static void buildTexture2dArray(QOpenGLFunctions_3_3_Core &gl,
                                const GLuint vbo,
                                const std::vector<GLuint> &srcNames,
                                const GLuint dstName,
                                const GLsizei img_width,
                                const GLsizei img_height)
{
    assert(vbo != 0);

    if (img_width != img_height)
        throw std::runtime_error("texture must be square");

    using GLsizeui = std::make_unsigned_t<GLsizei>;

    if (!utils::isPowerOfTwo(static_cast<GLsizeui>(img_width)))
        throw std::runtime_error("texture size must be a power of two");

    gl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo);
    gl.glBufferData(GL_PIXEL_UNPACK_BUFFER, img_width * img_height * 4, nullptr, GL_DYNAMIC_COPY);
    gl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // GLint oldTexture = xglGetInteger(GL_ACTIVE_TEXTURE);
    // GLint oldTex2d = xglGetInteger(GL_TEXTURE_BINDING_2D);
    // GLint oldTex2dArray = xglGetInteger(GL_TEXTURE_BINDING_2D_ARRAY);
    // GLint oldReadBuffer = xglGetInteger(GL_COPY_READ_BUFFER_BINDING);
    // GLint oldPackBuffer = xglGetInteger(GL_PIXEL_PACK_BUFFER_BINDING);
    // xglGetInteger(GL_PACK_ALIGNMENT);

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D_ARRAY, dstName);
    gl.glActiveTexture(GL_TEXTURE1);
    gl.glBindTexture(GL_TEXTURE_2D, 0);
    gl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo);
    gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, vbo);
    gl.glPixelStorei(GL_PACK_ALIGNMENT, 4);

    struct NODISCARD CleanupRaii final
    {
        QOpenGLFunctions_3_3_Core &m_gl;
        ~CleanupRaii()
        {
            m_gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            m_gl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            m_gl.glActiveTexture(GL_TEXTURE1);
            m_gl.glBindTexture(GL_TEXTURE_2D, 0);
            m_gl.glActiveTexture(GL_TEXTURE0);
            m_gl.glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }
    };
    CleanupRaii cleanup{gl};

    const GLsizei layers = static_cast<GLsizei>(srcNames.size());
    for (GLsizei layer = 0; layer < layers; ++layer) {
        const GLuint srcName = srcNames[static_cast<GLsizeui>(layer)];

        gl.glActiveTexture(GL_TEXTURE1);
        gl.glBindTexture(GL_TEXTURE_2D, srcName);

        const GLint baseLevel = xglGetTexParameteri(gl, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL);
        const GLint maxLevel = xglGetTexParameteri(gl, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL);
        MAYBE_UNUSED const GLint inFmt = xglGetTexLevelParameteri(gl,
                                                                  GL_TEXTURE_2D,
                                                                  0,
                                                                  GL_TEXTURE_INTERNAL_FORMAT);
        if (baseLevel != 0)
            throw std::runtime_error("baseLevel is not zero");

        if (maxLevel != 1000)
            throw std::runtime_error("maxLevel is not 1000");

        {
            const GLint width = xglGetTexLevelParameteri(gl, GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH);
            const GLint height = xglGetTexLevelParameteri(gl, GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT);

            if (img_width != width || img_height != height)
                throw std::runtime_error("input texture is the wrong size");
        }

        for (int level = 0; (img_width >> level) > 0; ++level) {
            gl.glActiveTexture(GL_TEXTURE1);
            const GLint level_width = xglGetTexLevelParameteri(gl,
                                                               GL_TEXTURE_2D,
                                                               level,
                                                               GL_TEXTURE_WIDTH);
            const GLint level_height = xglGetTexLevelParameteri(gl,
                                                                GL_TEXTURE_2D,
                                                                level,
                                                                GL_TEXTURE_HEIGHT);

            assert(level_width == level_height);
            assert(level_width == img_width >> level);

            // writes to GL_PIXEL_PACK_BUFFER
            gl.glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // reads from GL_PIXEL_UNPACK_BUFFER
            gl.glActiveTexture(GL_TEXTURE0);
            gl.glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                               level,
                               0,
                               0,
                               layer,
                               level_width,
                               level_height,
                               1,
                               GL_RGBA,
                               GL_UNSIGNED_BYTE,
                               nullptr);
        }
    }
}

} // namespace init_array_helper

void OpenGL::initArray(const SharedMMTexture &array, const std::vector<SharedMMTexture> &input)
{
    Legacy::VBO vbo;
    vbo.emplace(getSharedFunctions());

    MMTexture &tex = deref(array);
    QOpenGLTexture &qtex = deref(tex.get());

    std::vector<GLuint> srcNames;
    srcNames.reserve(input.size());
    for (const auto &x : input) {
        srcNames.emplace_back(deref(deref(x).get()).textureId());
    }
    assert(srcNames.size() == input.size());
    assert(qtex.layers() == static_cast<int>(input.size()));

    init_array_helper::buildTexture2dArray(getFunctions(),
                                           vbo.get(),
                                           srcNames,
                                           qtex.textureId(),
                                           qtex.width(),
                                           qtex.height());

    vbo.reset();
}
