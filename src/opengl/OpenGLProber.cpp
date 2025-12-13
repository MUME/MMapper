// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "OpenGLProber.h"

#include "../global/ConfigConsts.h"
#include "../global/logging.h"
#include "OpenGLConfig.h"

#include <optional>
#include <sstream>

#include <QOpenGLContext>

#ifdef WIN32
extern "C" {
// Prefer discrete nVidia and AMD GPUs by default on Windows
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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
        highestGLVersion = "Fallback";
    }
    return highestGLVersion;
}

NODISCARD QSurfaceFormat getOptimalFormat(std::optional<GLContextCheckResult> result)
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    if (result) {
        format.setVersion(result->version.major, result->version.minor);
        format.setProfile(result->isCore ? QSurfaceFormat::CoreProfile
                                         : QSurfaceFormat::CompatibilityProfile);
        QSurfaceFormat::FormatOptions options;
        if (result->isDebug) {
            options |= QSurfaceFormat::DebugContext;
        }
        if (result->isCompat) {
            options |= QSurfaceFormat::DeprecatedFunctions;
        }
        format.setOptions(options);
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

OpenGLProber::ProbeResult probeOpenGL()
{
    if constexpr (NO_OPENGL) {
        return {};
    }

    MMLOG_DEBUG() << "Probing for OpenGL support...";

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);

    QSurfaceFormat::FormatOptions optionsCompat = QSurfaceFormat::DebugContext
                                                  | QSurfaceFormat::DeprecatedFunctions;
    QSurfaceFormat::FormatOptions optionsCore = QSurfaceFormat::DebugContext;

    // Define lists of versions to try.
    std::vector<GLVersion> versions
        = {{4, 6}, {4, 5}, {4, 4}, {4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3}, {3, 2}};

    std::optional<GLContextCheckResult> coreResult = probeCore(format, versions, optionsCore);
    std::optional<GLContextCheckResult> compatResult = probeCompat(format,
                                                                   versions,
                                                                   optionsCompat,
                                                                   coreResult);

    OpenGLProber::ProbeResult result;
    if (compatResult || coreResult) {
        result.backendType = OpenGLProber::BackendType::GL;
        result.highestVersionString = getHighestGLVersion(coreResult, compatResult);
        OpenGLConfig::setGLVersionString(result.highestVersionString);
        result.format = getOptimalFormat(compatResult ? compatResult : coreResult);
        result.isCompat = (compatResult && compatResult->isCompat);
    }
    return result;
}

OpenGLProber::ProbeResult probeOpenGLES()
{
    if constexpr (NO_GLES) {
        return {};
    }

    MMLOG_DEBUG() << "Probing for OpenGL ES support...";

    std::vector<GLVersion> glesVersions = {{3, 2}, {3, 1}, {3, 0}};

    for (const auto &version : glesVersions) {
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setVersion(version.major, version.minor);

        QOpenGLContext context;
        context.setFormat(format);
        if (context.create()) {
            MMLOG_DEBUG() << "[GL Probe] Found highest supported GLES version: " << version;
            OpenGLProber::ProbeResult result;
            result.backendType = OpenGLProber::BackendType::GLES;
            result.format = format;
            std::ostringstream oss;
            oss << "ES" << version;
            result.highestVersionString = oss.str();
            OpenGLConfig::setESVersionString(result.highestVersionString);
            return result;
        }
    }

    MMLOG_DEBUG() << "No suitable GLES context found.";
    return {};
}

} // namespace

OpenGLProber::ProbeResult OpenGLProber::probe()
{
    auto glResult = probeOpenGL();
    auto glesResult = probeOpenGLES();
    if (glResult.backendType != BackendType::None) {
        return glResult;
    }
    if (glesResult.backendType != BackendType::None) {
        return glesResult;
    }
    MMLOG_DEBUG() << "No suitable backend found.";
    return {};
}
