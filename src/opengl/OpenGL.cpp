// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "OpenGL.h"

#include "../global/ConfigConsts.h"
#include "../global/logging.h"
#include "./legacy/Legacy.h"
#include "./legacy/Meshes.h"
#include "OpenGLTypes.h"

#include <cassert>
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
            format.setVersion(2, 1);
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
        format.setVersion(2, 1);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setOptions(QSurfaceFormat::DebugContext | QSurfaceFormat::DeprecatedFunctions);
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
    std::vector<GLVersion> compatVersions = coreVersions;
    compatVersions.insert(compatVersions.end(), {{3, 1}, {3, 0}, {2, 1}}); // Add versions < 3.2

    std::optional<GLContextCheckResult> coreResult = probeCore(format, coreVersions, optionsCore);
    std::optional<GLContextCheckResult> compatResult = probeCompat(format,
                                                                   compatVersions,
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
