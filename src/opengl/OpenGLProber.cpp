// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "OpenGLProber.h"

#include "../global/ConfigConsts.h"
#include "../global/TextUtils.h"
#include "../global/logging.h"
#include "OpenGLConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#ifndef Q_OS_WASM
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QProcess>
#include <QSurfaceFormat>
#endif
#include <QString>

#ifdef WIN32
#include <windows.h>

// Prefer discrete nVidia and AMD GPUs by default on Windows
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

namespace {

OpenGLProber::ProbeResult getFallbackResult()
{
    OpenGLProber::ProbeResult result;
    result.backendType = OpenGLProber::BackendType::GL;
    result.format.setRenderableType(QSurfaceFormat::OpenGL);
    result.format.setVersion(3, 3);
    result.format.setProfile(QSurfaceFormat::CoreProfile);
    result.format.setDepthBufferSize(24);
    result.highestVersionString = "Fallback";
    return result;
}

} // namespace

OpenGLProber::ProbeResult OpenGLProber::probe()
{
#ifdef Q_OS_WASM
    MMLOG_DEBUG() << "Probing for OpenGL support (Wasm/WebGL 2.0)...";
    ProbeResult result;
    result.backendType = BackendType::ES;
    result.format.setRenderableType(QSurfaceFormat::OpenGLES);
    result.format.setVersion(3, 0);
    result.format.setDepthBufferSize(24);
    result.highestVersionString = "WebGL 2.0";
    OpenGLConfig::setESVersionString(result.highestVersionString);
    return result;
#else
    if constexpr (NO_OPENGL && NO_GLES) {
        return {};
    }

    MMLOG_DEBUG() << "Probing for OpenGL support via --probe subprocess...";

    const QString selfPath = QCoreApplication::applicationFilePath();
    QProcess process;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ASAN_OPTIONS", "detect_leaks=0");
    process.setProcessEnvironment(env);

    QStringList args = {"--probe"};
    const QString platform = QGuiApplication::platformName();
    if (!platform.isEmpty()) {
        args << "-platform" << platform;
    }
    process.start(selfPath, args);
    if (!process.waitForFinished(5000)) {
        MMLOG_ERROR() << "OpenGL probe subprocess timed out or failed to start. Using fallback.";
        process.kill();
        return getFallbackResult();
    }

    const QByteArray stdoutData = process.readAllStandardOutput();
    const QByteArray stderrData = process.readAllStandardError();
    return parseSurveyResult(stdoutData, stderrData);
#endif
}

OpenGLProber::ProbeResult OpenGLProber::parseSurveyResult(const QByteArray &stdoutData,
                                                          const QByteArray &stderrData)
{
    const qsizetype start = stdoutData.indexOf('{');
    const qsizetype end = stdoutData.lastIndexOf('}');

    QJsonDocument doc;
    if (start != -1 && end != -1 && end > start) {
        doc = QJsonDocument::fromJson(stdoutData.mid(start, end - start + 1));
    }

    if (doc.isNull() || !doc.isObject()) {
        MMLOG_WARNING() << "OpenGL survey returned invalid JSON. Using fallback."
                        << "\n  stdout:" << stdoutData.toStdString()
                        << "\n  stderr:" << stderrData.toStdString();
        return getFallbackResult();
    }

    QJsonObject obj = doc.object();
    QString backend = obj["backend"].toString();
    const bool debugSupported = obj["debug"].toBool();

    ProbeResult result;
    result.debugSupported = debugSupported;
    if (debugSupported) {
        result.format.setOption(QSurfaceFormat::DebugContext);
    }

    if (backend == "GL") {
        result.backendType = BackendType::GL;
        int major = obj["major"].toInt();
        int minor = obj["minor"].toInt();
        result.format.setRenderableType(QSurfaceFormat::OpenGL);
        result.format.setVersion(major, minor);
        result.format.setProfile(QSurfaceFormat::CoreProfile);
        result.format.setDepthBufferSize(24);
        result.highestVersionString = mmqt::toStdStringUtf8(
            QString("GL%1.%2").arg(major).arg(minor));
        OpenGLConfig::setGLVersionString(result.highestVersionString);
    } else if (backend == "ES") {
        result.backendType = BackendType::GLES;
        int major = obj["major"].toInt();
        int minor = obj["minor"].toInt();
        result.format.setRenderableType(QSurfaceFormat::OpenGLES);
        result.format.setVersion(major, minor);
        result.format.setDepthBufferSize(24);
        result.highestVersionString = mmqt::toStdStringUtf8(
            QString("ES%1.%2").arg(major).arg(minor));
        OpenGLConfig::setESVersionString(result.highestVersionString);
    } else {
        MMLOG_DEBUG() << "No suitable backend found by survey.";
        return {};
    }

    MMLOG_INFO() << "[GL Probe] Survey determined: " << result.highestVersionString.c_str();
    return result;
}

#ifndef Q_OS_WASM
int OpenGLProber::runSurveyMode(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QJsonObject result;

    struct GLVersion
    {
        int major;
        int minor;
    };

    // Try OpenGL Core versions from highest to lowest
    const std::vector<GLVersion> coreVersions
        = {{4, 6}, {4, 5}, {4, 4}, {4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3}};

    bool foundBackend = false;
    for (const auto &v : coreVersions) {
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setVersion(v.major, v.minor);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setOption(QSurfaceFormat::DebugContext);

        QOpenGLContext context;
        context.setFormat(format);
        if (context.create()) {
            const QSurfaceFormat actual = context.format();
            if (actual.profile() == QSurfaceFormat::CoreProfile
                && (actual.majorVersion() > v.major
                    || (actual.majorVersion() == v.major && actual.minorVersion() >= v.minor))) {
                result["backend"] = "GL";
                result["major"] = actual.majorVersion();
                result["minor"] = actual.minorVersion();
                result["debug"] = actual.testOption(QSurfaceFormat::DebugContext);
                foundBackend = true;
                break;
            }
        }
    }

    if (!foundBackend) {
        const std::vector<GLVersion> glesVersions = {{3, 2}, {3, 1}, {3, 0}};
        for (const auto &v : glesVersions) {
            QSurfaceFormat format;
            format.setRenderableType(QSurfaceFormat::OpenGLES);
            format.setVersion(v.major, v.minor);
            format.setOption(QSurfaceFormat::DebugContext);

            QOpenGLContext context;
            context.setFormat(format);
            if (context.create()) {
                const QSurfaceFormat actual = context.format();
                result["backend"] = "ES";
                result["major"] = actual.majorVersion();
                result["minor"] = actual.minorVersion();
                result["debug"] = actual.testOption(QSurfaceFormat::DebugContext);
                foundBackend = true;
                break;
            }
        }
    }

    if (result.isEmpty()) {
        result["backend"] = "None";
    }

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Compact);
    fprintf(stdout, "\n%s\n", output.constData());
    fflush(stdout);
    return 0;
}
#endif
