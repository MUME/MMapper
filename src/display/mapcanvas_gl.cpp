// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../global/Array.h"
#include "../global/CaseUtils.h"
#include "../global/ChangeMonitor.h"
#include "../global/ConfigConsts.h"
#include "../global/RuleOf5.h"
#include "../global/logging.h"
#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "../map/coordinate.h"
#include "../mapdata/mapdata.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLConfig.h"
#include "../opengl/OpenGLTypes.h"
#include "../opengl/legacy/Meshes.h"
#include "../opengl/legacy/TFO.h"
#include "../opengl/legacy/VAO.h"
#include "../opengl/legacy/VBO.h"
#include "../src/global/SendToUser.h"
#include "Connections.h"
#include "MapCanvasConfig.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "ProjectionUtils.h"
#include "Textures.h"
#include "connectionselection.h"
#include "mapcanvas.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef Q_OS_WASM
#include <emscripten/heap.h>
#endif

#include <QMessageLogContext>
#include <QOpenGLContext>
#include <QtCore>
#include <QtGui/qopengl.h>
#include <QtGui>

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT // added in OpenGL 4.3
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x2
#endif
#ifndef GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT // added in OpenGL 4.3
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT 0x4
#endif
#ifndef GL_CONTEXT_FLAG_NO_ERROR_BIT // added in OpenGL 4.6
#define GL_CONTEXT_FLAG_NO_ERROR_BIT 0x8
#endif

namespace MapCanvasConfig {

void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                            ChangeMonitor::Function callback)
{
    return setConfig().canvas.advanced.registerChangeCallback(lifetime, std::move(callback));
}

bool isIn3dMode()
{
    return getConfig().canvas.advanced.use3D.get();
}

void set3dMode(bool is3d)
{
    setConfig().canvas.advanced.use3D.set(is3d);
}

bool isAutoTilt()
{
    return getConfig().canvas.advanced.autoTilt.get();
}

void setAutoTilt(const bool val)
{
    setConfig().canvas.advanced.autoTilt.set(val);
}

bool getShowPerfStats()
{
    return getConfig().canvas.advanced.printPerfStats.get();
}

void setShowPerfStats(const bool show)
{
    setConfig().canvas.advanced.printPerfStats.set(show);
}

} // namespace MapCanvasConfig

void MapCanvas::hostCleanupGL()
{
    // NOTE: The caller (the host facade) is responsible for making sure a GL
    // context is current before calling this function.
    const auto fname = MM_SOURCE_LOCATION().function_name();
    qInfo() << fname << "Entered.";

    if (std::exchange(m_cleanedUp, true)) {
        qInfo() << fname << "Skipping duplicate cleanup!";
        return;
    }

    qInfo() << fname << "Cleaning up...";

    // note: m_batchedMeshes co-owns textures created by MapCanvasData,
    // and it also owns the lifetime of some OpenGL objects (e.g. VBOs).
    m_batches.resetExistingMeshesAndIgnorePendingRemesh();
    m_weather.cleanup();
    m_textures.destroyAll();
    getGLFont().cleanup();
    getOpenGL().cleanup();
    m_logger.reset();

    qInfo() << fname << "Done.";
}

void MapCanvas::reportGLVersion()
{
    auto &gl = getOpenGL();

    auto logMsg = [this](const QByteArray &prefix, const QByteArray &msg) -> void {
        qInfo() << prefix << msg;
        emit sig_log("MapCanvas", prefix + " " + msg);
    };

    auto getString = [&gl](const GLenum name) -> QByteArray {
        return QByteArray{gl.glGetString(name)};
    };

    auto logString = [&getString, &logMsg](const QByteArray &prefix, const GLenum name) -> void {
        logMsg(prefix, getString(name));
    };

    static auto checkBit =
        [](std::vector<std::string> &v, GLint &flags, const GLint bit, const std::string_view what) {
            static_assert(sizeof(bit) == sizeof(uint32_t));
            assert(utils::isPowerOfTwo(static_cast<uint32_t>(bit)));
            if (flags & bit) {
                flags &= ~bit;
                v.emplace_back(toLowerUtf8(what));
            }
        };

    auto logVector =
        [&logMsg](const QByteArray &thing, std::vector<std::string> &v, const GLint flags) {
            if (flags != 0) {
                v.emplace_back(std::to_string(flags));
            }

            std::ostringstream oss;
            oss << "[";
            auto prefix = "";
            for (const std::string_view what : v) {
                oss << prefix << what;
                prefix = " | ";
            }
            oss << "]";
            const auto str = std::move(oss).str();
            logMsg(thing, mmqt::toQByteArrayUtf8(str));
        };

    logString("OpenGL Version:", GL_VERSION);
    logString("OpenGL Renderer:", GL_RENDERER);
    logString("OpenGL Vendor:", GL_VENDOR);
    logString("OpenGL GLSL:", GL_SHADING_LANGUAGE_VERSION);

#ifndef Q_OS_WASM
    {
        GLint profileMask = gl.glGetInteger(GL_CONTEXT_PROFILE_MASK);
        std::vector<std::string> v;

#define X_CHECK(_x) checkBit(v, profileMask, (GL_CONTEXT_##_x##_PROFILE_BIT), (#_x))
        X_CHECK(COMPATIBILITY);
        X_CHECK(CORE);
#undef X_CHECK

        logVector("OpenGL context profile mask: ", v, profileMask);
    }
    {
        GLint flags = gl.glGetInteger(GL_CONTEXT_FLAGS);
        std::vector<std::string> v;

#define X_CHECK(_x) checkBit(v, flags, (GL_CONTEXT_FLAG_##_x##_BIT), (#_x))
        X_CHECK(DEBUG);
        X_CHECK(FORWARD_COMPATIBLE);
        X_CHECK(NO_ERROR);
        X_CHECK(ROBUST_ACCESS);
#undef X_CHECK

        logVector("OpenGL context flags: ", v, flags);
    }
#endif

    // NOTE: hostInitializeGL() runs with a current GL context (the host
    // guarantees this, same as the old QOpenGLWindow::initializeGL()), so
    // querying the thread's current context here is equivalent to the old
    // QOpenGLWindow::context() call, without depending on any particular host.
    const auto version = std::invoke([]() -> std::string {
        std::ostringstream oss;
        if (auto *const ctxt = QOpenGLContext::currentContext()) {
            const QSurfaceFormat &format = ctxt->format();
            switch (format.renderableType()) {
            case QSurfaceFormat::OpenGL:
                oss << "GL";
                break;
            case QSurfaceFormat::OpenGLES:
                oss << "ES";
                break;
            case QSurfaceFormat::OpenVG:
                oss << "VG";
                break;
            case QSurfaceFormat::DefaultRenderableType:
            default:
                oss << "UN";
                break;
            }
            oss << format.majorVersion() << "." << format.minorVersion();
        } else {
            oss << "UNKNOWN";
        }
        return std::move(oss).str();
    });

    logMsg("Current OpenGL Context:",
           QString("%1 (%2)")
               .arg(version.c_str())
               // FIXME: This is a bit late to report an invalid context.
               .arg((QOpenGLContext::currentContext() != nullptr
                     && QOpenGLContext::currentContext()->isValid())
                        ? "valid"
                        : "invalid")
               .toUtf8());
    if constexpr (!NO_OPENGL) {
        logMsg("Highest OpenGL:", mmqt::toQByteArrayUtf8(OpenGLConfig::getGLVersionString()));
    }
    if constexpr (!NO_GLES) {
        logMsg("Highest GLES:", mmqt::toQByteArrayUtf8(OpenGLConfig::getESVersionString()));
    }

    logMsg("Display:",
           QString("%1 DPI, render scale %2%")
               .arg(currentDpr())
               .arg(getConfig().canvas.renderScale.get())
               .toUtf8());
}

bool MapCanvas::isBlacklistedDriver()
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        auto &gl = getOpenGL();
        auto getString = [&gl](const GLenum name) -> QByteArray {
            return QByteArray{gl.glGetString(name)};
        };

        const QByteArray &vendor = getString(GL_VENDOR);
        const QByteArray &renderer = getString(GL_RENDERER);
        if (vendor == "Microsoft Corporation" && renderer == "GDI Generic") {
            return true;
        }
    }

    return false;
}

bool MapCanvas::hostInitializeGL()
{
    OpenGL &gl = getOpenGL();
    try {
        gl.initializeOpenGLFunctions();

        // TODO: Perform the blacklist test as a call from main() to minimize player headache.
        if (isBlacklistedDriver()) {
            throw std::runtime_error("unsupported driver");
        }
    } catch (const std::exception &ex) {
        // The core stays QtWidgets-free, so it can't show the message box or
        // hide the host window itself; the host facade does that in response
        // to this signal.
        emit sig_glInitFailed(QString::fromUtf8(ex.what()));
        return false;
    }

    reportGLVersion();

    // NOTE: If you're adding code that relies on generating OpenGL errors (e.g. ANGLE),
    // you *MUST* force it to complete those error probes before calling initLogger(),
    // because the logger purposely calls std::abort() when it receives an error.
    initLogger();

    gl.initializeRenderer(static_cast<float>(currentDpr()));
    gl.setRenderScale(configuredRenderScale());

    gl.getUboManager()
        .registerRebuildFunction(Legacy::SharedVboEnum::NamedColorsBlock,
                                 [](Legacy::Functions &funcs) {
                                     auto &uboManager = funcs.getUboManager();
                                     uboManager.update<Legacy::SharedVboEnum::NamedColorsBlock>(
                                         funcs, XNamedColor::getAllColorsAsBlock());
                                 });

    gl.getUboManager().registerRebuildFunction(
        Legacy::SharedVboEnum::CameraBlock, [this](Legacy::Functions &funcs) {
            auto &camera = funcs.getUboManager().get<Legacy::SharedVboEnum::CameraBlock>();
            const auto playerPosCoord = m_data.tryGetPosition().value_or(Coordinate{0, 0, 0});
            camera.viewProj = getViewProj();
            camera.playerPos = glm::vec4(static_cast<float>(playerPosCoord.x),
                                         static_cast<float>(playerPosCoord.y),
                                         static_cast<float>(playerPosCoord.z),
                                         ProjectionUtils::ROOM_Z_SCALE);
            funcs.getUboManager().sync<Legacy::SharedVboEnum::CameraBlock>(funcs);
        });

    updateMultisampling();

    // REVISIT: should the font texture have the lowest ID?
    initTextures();
    auto &font = getGLFont();
    font.setTextureId(allocateTextureId());
    font.init(currentLogicalDpi());
    updateTextures();

    // compile all shaders
    {
        auto &sharedFuncs = gl.getSharedFunctions(Badge<MapCanvas>{});
        Legacy::Functions &funcs = deref(sharedFuncs);
        Legacy::ShaderPrograms &programs = funcs.getShaderPrograms();
        programs.early_init();
    }

    setConfig().canvas.showUnsavedChanges.registerChangeCallback(m_lifetime, [this]() {
        if (getConfig().canvas.showUnsavedChanges.get() && m_diff.highlight.has_value()
            && m_diff.highlight->highlights.empty()) {
            this->requestForceUpdateMeshes();
        }
    });

    setConfig().canvas.showMissingMapId.registerChangeCallback(m_lifetime, [this]() {
        if (getConfig().canvas.showMissingMapId.get() && m_diff.highlight.has_value()
            && m_diff.highlight->highlights.empty()) {
            this->requestForceUpdateMeshes();
        }
    });

    setConfig().canvas.showUnmappedExits.registerChangeCallback(m_lifetime, [this]() {
        this->requestForceUpdateMeshes();
    });

    setConfig().canvas.antialiasingSamples.registerChangeCallback(m_lifetime, [this]() {
        markMultisamplingDirty();
        m_frameManager.requestUpdate();
    });

    setConfig().canvas.renderScale.registerChangeCallback(m_lifetime, [this]() {
        // Deferred like m_pendingDpr: the FBO is reconfigured at the top of
        // the next paint, where the GL context is current.
        m_pendingRenderScale = configuredRenderScale();
        m_frameManager.requestUpdate();
    });

    setConfig().canvas.trilinearFiltering.registerChangeCallback(m_lifetime, [this]() {
        this->requestUpdateTextures();
    });

    const auto requestFontReload = [this]() {
        // Deferred like m_pendingRenderScale: needs the GL context.
        m_pendingFontReload = true;
        m_frameManager.requestUpdate();
    };
    setConfig().canvas.mapFontFamily.registerChangeCallback(m_lifetime, requestFontReload);
    setConfig().canvas.mapFontPointSize.registerChangeCallback(m_lifetime, requestFontReload);

    // NOTE: The host facade is responsible for connecting to
    // QOpenGLContext::aboutToBeDestroyed (see MapCanvas::initializeGL()); the
    // core has no notion of a QOpenGLContext or its lifetime.
    return true;
}

/* Direct means it is always called from the emitter's thread */
void MapCanvas::slot_onMessageLoggedDirect(const QOpenGLDebugMessage &message)
{
    using Type = QOpenGLDebugMessage::Type;
    switch (message.type()) {
    case Type::InvalidType:
    case Type::ErrorType:
    case Type::UndefinedBehaviorType:
        break;
    case Type::DeprecatedBehaviorType:
    case Type::PortabilityType:
    case Type::PerformanceType:
    case Type::OtherType:
    case Type::MarkerType:
    case Type::GroupPushType:
    case Type::GroupPopType:
    case Type::AnyType:
        qWarning() << message;
        return;
    }

    qCritical() << message;

    // The core stays QtWidgets-free, so it can't show the blocking message
    // box itself; the host facade does that (and calls std::abort()) in
    // response to this signal, using a direct connection so the abort still
    // happens synchronously from here.
    emit sig_glFatalError(message.message());
}

void MapCanvas::initLogger()
{
    m_logger = std::make_unique<QOpenGLDebugLogger>(this);
    connect(m_logger.get(),
            &QOpenGLDebugLogger::messageLogged,
            this,
            &MapCanvas::slot_onMessageLoggedDirect,
            Qt::DirectConnection /* NOTE: executed in emitter's thread */);

    if (!m_logger->initialize()) {
        m_logger.reset();
        qWarning() << "Failed to initialize OpenGL debug logger";
        return;
    }

    m_logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
    m_logger->disableMessages();
    m_logger->enableMessages(QOpenGLDebugMessage::AnySource,
                             (QOpenGLDebugMessage::ErrorType
                              | QOpenGLDebugMessage::UndefinedBehaviorType),
                             QOpenGLDebugMessage::AnySeverity);
}

void MapCanvas::setMvp(const glm::mat4 &viewProj)
{
    auto &gl = getOpenGL();
    // Pushes the externally provided projection matrix into the viewport cache,
    // which also ensures the dirty flag is cleared for the current state.
    setMvpExtern(viewProj);
    gl.setProjectionMatrix(viewProj);
}

void MapCanvas::setViewportAndMvp(const int width, const int height, const qreal dpr)
{
    // Refresh the cached viewport geometry (and DPR) every time; this is called
    // once per paint (from actuallyPaintGL()) as well as from hostResize(), so it
    // also acts as the "first frame" safety net, since nothing else queries
    // the live host size on every access.
    setViewportSize(width, height, dpr);

    if (width != m_lastWidth || height != m_lastHeight) {
        m_lastWidth = width;
        m_lastHeight = height;
        markViewProjDirty();
    }

    auto &gl = getOpenGL();
    gl.glViewport(0, 0, width, height);

    const auto size = getViewport().size;
    assert(size.x == width);
    assert(size.y == height);

    gl.setProjectionMatrix(MapCanvasViewport::getViewProj());
}

void MapCanvas::onViewProjDirty() const
{
    m_opengl.getUboManager().invalidate(Legacy::SharedVboEnum::CameraBlock);
}

void MapCanvas::hostResize(const int width, const int height, const qreal dpr)
{
    if (m_textures.room_highlight == nullptr) {
        // hostResize called but hostInitializeGL was not called yet
        return;
    }

    setViewportAndMvp(width, height, dpr);
    markMultisamplingDirty();
    m_frameManager.requestUpdate();
}

void MapCanvas::updateBatches()
{
    updateMapBatches();
    updateInfomarkBatches();
}

void MapCanvas::updateMapBatches()
{
    RemeshCookie &remeshCookie = m_batches.remeshCookie;
    if (remeshCookie.isPending()) {
        return;
    }

    if (m_batches.mapBatches.has_value() && !m_data.getNeedsMapUpdate()) {
        return;
    }

    if (m_data.getNeedsMapUpdate()) {
        m_data.clearNeedsMapUpdate();
        assert(!m_data.getNeedsMapUpdate());
        MMLOG() << "[updateMapBatches] cleared 'needsUpdate' flag";
    }

    auto getFuture = [this]() {
        MMLOG() << "[updateMapBatches] calling generateBatches";
        return m_data.generateBatches(mctp::getProxy(m_textures),
                                      getGLFont().getSharedFontMetrics());
    };

    remeshCookie.set(getFuture());
    assert(remeshCookie.isPending());

    m_diff.cancelUpdates(m_data.getSavedMap());
}

bool Batches::isInProgress() const
{
    return remeshCookie.isPending() || next_mapBatches.has_value();
}

void MapCanvas::finishPendingMapBatches()
{
    if (!m_batches.isInProgress()) {
        return;
    }

#define LOG() MMLOG() << prefix
    static const std::string_view prefix = "[finishPendingMapBatches] ";

    if (m_batches.next_mapBatches.has_value()) {
        m_batches.mapBatches = std::exchange(m_batches.next_mapBatches, std::nullopt);
    }

    RemeshCookie &remeshCookie = m_batches.remeshCookie;
    if (!remeshCookie.isPending() || !remeshCookie.isReady()) {
        return;
    }

    LOG() << "Waiting for the cookie. This shouldn't take long.";
    try {
        SharedMapBatchFinisher pFuture = remeshCookie.get();
        assert(!remeshCookie.isPending());

        if (pFuture == nullptr) {
            // REVISIT: Do we need to schedule another update now?
            LOG() << "Got NULL (means the update was flagged to be ignored)";
            return;
        }

        // REVISIT: should we pass a "fake" one and only swap to the correct one on success?
        LOG() << "Clearing the map batches and call the finisher to create new ones";

        DECL_TIMER(t, __FUNCTION__);
        const IMapBatchesFinisher &future = *pFuture;
        std::optional<MapBatches> &opt_mapBatches = m_batches.next_mapBatches;
        opt_mapBatches.reset();
        finish(future, opt_mapBatches, getOpenGL(), getGLFont());
        assert(opt_mapBatches.has_value());
        m_data.saveSnapshot();

        // Swap immediately so this frame can use the new batches.
        m_batches.mapBatches = std::exchange(m_batches.next_mapBatches, std::nullopt);
    } catch (...) {
        QString msg;
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception &ex) {
            msg = ex.what();
        } catch (...) {
            msg = QStringLiteral("unknown");
        }
        const auto s = QString("ERROR: %1\nReverting map to previous snapshot. Please file a bug!\n")
                           .arg(msg);
        qWarning().noquote() << s;
        global::sendToUser(s);

        // FIXME: This causes a cycle when the remeshing throws.
        m_data.restoreSnapshot();
    }
#undef LOG
}

void MapCanvas::applyPendingGLWork()
{
    // Deferred (defer-to-next-render) GL work relies on the host's GL context
    // being current, so it is applied at the top of the paint, before the
    // batches are (re)built: a forced remesh must not discard the remesh
    // that updateBatches() just started, and glyph meshes must use the
    // font metrics of the new DPI / render scale.
    if (m_pendingDpr.has_value()) {
        const float newDpi = *m_pendingDpr;
        m_pendingDpr.reset();
        getOpenGL().setDevicePixelRatio(newDpi);
        auto &font = getGLFont();
        font.cleanup();
        font.init(currentLogicalDpi());
    }
    if (m_pendingRenderScale.has_value()) {
        const float newScale = *m_pendingRenderScale;
        m_pendingRenderScale.reset();
        if (!utils::isSameFloat(newScale, getOpenGL().getRenderScale())) {
            log(QString("Render scale: %1%").arg(static_cast<double>(newScale) * 100.0));
            getOpenGL().setRenderScale(newScale);
            // Glyphs are rasterized for the render target's pixel density,
            // and the FBO is sized from it.
            auto &font = getGLFont();
            font.cleanup();
            font.init(currentLogicalDpi());
            m_batches.resetExistingMeshesButKeepPendingRemesh();
            markMultisamplingDirty();
        }
    }
    if (std::exchange(m_pendingUpdateTextures, false)) {
        updateTextures();
    }
    if (std::exchange(m_pendingFontReload, false)) {
        auto &font = getGLFont();
        font.cleanup();
        font.init(currentLogicalDpi());
        // existing text meshes reference the old atlas
        m_pendingForceUpdateMeshes = true;
    }
    if (std::exchange(m_pendingForceUpdateMeshes, false)) {
        forceUpdateMeshes();
    }
}

void MapCanvas::actuallyPaintGL()
{
    // DECL_TIMER(t, __FUNCTION__);

    const QSize hostSizeNow = currentHostSize();
    setViewportAndMvp(hostSizeNow.width(), hostSizeNow.height(), currentDpr());
    if (takeMultisamplingDirty()) {
        updateMultisampling();
    }

    auto &gl = getOpenGL();
    auto &funcs = deref(gl.getSharedFunctions(Badge<MapCanvas>{}));

    gl.getUboManager().bind(funcs, Legacy::SharedVboEnum::NamedColorsBlock);

    gl.bindFbo();
    bool paintSucceeded = true;
    {
        // Scoped tightly around the paint calls so releaseFbo() always runs
        // here, before blitFboToDefault() (which requires the FBO to be
        // unbound), regardless of whether painting threw.
        struct FboReleaseGuard final
        {
            OpenGL &gl;
            ~FboReleaseGuard() { gl.releaseFbo(); }
        } releaseGuard{gl};

        try {
            gl.clear(Color{getConfig().canvas.backgroundColor});

            if (m_data.isEmpty()) {
                getGLFont().renderTextCentered("No map loaded");
            } else {
                // Update animation state
                m_weather.update();

                paintMap();
                paintBatchedInfomarks();
                paintSelections();
                paintCharacters();
                paintDifferences();

                m_weather.prepare();
                gl.getUboManager().bind(funcs, Legacy::SharedVboEnum::TimeBlock);

                m_weather.render(m_opengl.getDefaultRenderState());
            }
        } catch (...) {
            paintSucceeded = false;
            QString msg;
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception &ex) {
                msg = ex.what();
            } catch (...) {
                msg = QStringLiteral("unknown");
            }
            const auto s = QString("ERROR during paint: %1\n").arg(msg);
            qWarning().noquote() << s;
            global::sendToUser(s);
        }
    }

    if (paintSucceeded) {
        m_host.applyPresentViewport();
        gl.blitFboToDefault();
    }
}

NODISCARD bool MapCanvas::Diff::isUpToDate(const Map &saved, const Map &current) const
{
    return highlight && highlight->saved.isSamePointer(saved)
           && highlight->current.isSamePointer(current);
}

// this differs from isUpToDate in that it allows display of a diff based on the current saved map,
// but it allows the "current" to be different (e.g. during the async remesh for the current map).
NODISCARD bool MapCanvas::Diff::hasRelatedDiff(const Map &saved) const
{
    return highlight && highlight->saved.isSamePointer(saved);
}

void MapCanvas::Diff::cancelUpdates(const Map &saved)
{
    futureHighlight.reset();
    if (highlight) {
        if (!hasRelatedDiff(saved)) {
            highlight.reset();
        }
    }
}

void MapCanvas::Diff::maybeAsyncUpdate(const Map &saved, const Map &current)
{
    auto &diff = *this;

    // Pending takes precedence. This also usually guarantees at most one pending update at a time,
    // but calling resetExistingMeshesAndIgnorePendingRemesh() could result in more than one diff
    // mesh thread executing concurrently, where the old one will be ignored.
    if (diff.futureHighlight) {
        constexpr auto immediate = std::chrono::milliseconds(0);
        if (diff.futureHighlight->wait_for(immediate) != std::future_status::timeout) {
            try {
                diff.highlight = diff.futureHighlight->get();
            } catch (const std::exception &ex) {
                MMLOG_ERROR() << "Exception: " << ex.what();
            }
            diff.futureHighlight.reset();
        }
        return;
    }

    // no change necessary
    if (isUpToDate(saved, current)) {
        return;
    }

    const auto &config = getConfig();
    const auto &canvas = config.canvas;
    const bool showNeedsServerId = canvas.showMissingMapId.get();
    const bool showChanged = canvas.showUnsavedChanges.get();

    diff.futureHighlight = std::async(
        std::launch::async,
        [saved, current, showNeedsServerId, showChanged]() -> Diff::HighlightDiff {
            DECL_TIMER(t2,
                       "[async] actuallyPaintGL: highlight changes, temporary, and needs update");

            auto getHighlights =
                [&saved, &current, showChanged, showNeedsServerId]() -> Diff::MaybeDataOrMesh {
                if (!showChanged && !showNeedsServerId) {
                    return Diff::MaybeDataOrMesh{};
                }

                DECL_TIMER(t3, "[async] actuallyPaintGL: compute highlights");
                DiffQuadVector highlights;
                auto drawQuad = [&highlights](const RawRoom &room, const NamedColorEnum color) {
                    const auto pos = room.getPosition().to_ivec3();
                    highlights.emplace_back(pos, 0, color);
                };

                // Handle rooms needing a server ID or that are temporary
                if (showNeedsServerId) {
                    current.getRooms().for_each([&current, &drawQuad](auto id) {
                        if (auto h = current.getRoomHandle(id)) {
                            if (h.isTemporary()) {
                                drawQuad(h.getRaw(), NamedColorEnum::HIGHLIGHT_TEMPORARY);
                            } else if (h.getServerId() == INVALID_SERVER_ROOMID) {
                                drawQuad(h.getRaw(), NamedColorEnum::HIGHLIGHT_NEEDS_SERVER_ID);
                            }
                        }
                    });
                }

                // Handle changed rooms
                if (showChanged) {
                    ProgressCounter dummyPc;
                    Map::foreachChangedRoom(dummyPc,
                                            saved,
                                            current,
                                            [&drawQuad](const RawRoom &room) {
                                                drawQuad(room, NamedColorEnum::HIGHLIGHT_UNSAVED);
                                            });
                }

                if (highlights.empty()) {
                    return Diff::MaybeDataOrMesh{};
                }
                return Diff::MaybeDataOrMesh{std::move(highlights)};
            };

            return Diff::HighlightDiff{saved, current, getHighlights()};
        });
}

void MapCanvas::paintDifferences()
{
    auto &diff = m_diff;
    const auto &saved = m_data.getSavedMap();
    const auto &current = m_data.getCurrentMap();

    diff.maybeAsyncUpdate(saved, current);
    if (!diff.hasRelatedDiff(saved)) {
        return;
    }

    auto &highlight = deref(diff.highlight);
    auto &gl = getOpenGL();

    if (auto &highlights = highlight.highlights; !highlights.empty()) {
        highlights.render(gl, m_textures.room_highlight->getArrayPosition().array);
    }
}

void MapCanvas::paintMap()
{
    const bool pending = m_batches.remeshCookie.isPending();

    if (!m_batches.mapBatches.has_value()) {
        if (!pending || m_batches.pendingUpdateFlashState.tick()) {
            const QString msg = pending ? "Please wait... the map isn't ready yet." : "Batch error";
            getGLFont().renderTextCentered(msg);
        }
        if (!pending) {
            // REVISIT: does this need a better fix?
            // pending already scheduled an update, but now we realize we need an update.
            m_frameManager.requestUpdate();
        }
        return;
    }

    // TODO: add a GUI indicator for pending update?
    renderMapBatches();

    if (pending) {
        if (m_batches.pendingUpdateFlashState.tick()) {
            const QString msg = "CAUTION: Async map update pending!";
            getGLFont().renderTextCentered(msg);
        }
    }
}

void MapCanvas::paintSelections()
{
    paintSelectedRooms();
    paintSelectedConnection();
    paintSelectionArea();
    paintSelectedInfomarks();
}

void MapCanvas::hostPaintGL()
{
    auto frame = m_frameManager.beginFrame();
    if (!frame) {
        // Blit the existing FBO on resize or expose
        m_host.applyPresentViewport();
        getOpenGL().blitFboToDefault();
        return;
    }

    static thread_local double longestBatchMs = 0.0;

    const bool showPerfStats = MapCanvasConfig::getShowPerfStats();

    using Clock = std::chrono::high_resolution_clock;
    std::optional<Clock::time_point> optStart;
    std::optional<Clock::time_point> optAfterTextures;
    std::optional<Clock::time_point> optAfterBatches;
    if (showPerfStats) {
        optStart = Clock::now();
    }

    {
        applyPendingGLWork();
        if (showPerfStats) {
            optAfterTextures = Clock::now();
        }

        // Note: The real work happens here!
        updateBatches();

        // And here
        finishPendingMapBatches();

        // For accurate timing of the update, we'd need to call glFinish(),
        // or at least set up an OpenGL query object. The update will send
        // a lot of data to the GPU, so it could take a while...
        if (showPerfStats) {
            optAfterBatches = Clock::now();
        }

        actuallyPaintGL();
    }

    if (!showPerfStats) {
        return; /* don't wait to finish */
    }

    const auto &start = optStart.value();
    const auto &afterTextures = optAfterTextures.value();
    const auto &afterBatches = optAfterBatches.value();
    const auto afterPaint = Clock::now();
    auto &gl = getOpenGL();
    const bool calledFinish = std::invoke([&gl]() -> bool {
        if (!gl.isRendererInitialized()) {
            return false;
        }
        gl.glFinish();
        return true;
    });

    const auto end = Clock::now();

    const auto ms = [](auto delta) -> double {
        return double(std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count()) * 1e-6;
    };

    const auto w = width();
    const auto h = height();
    const auto dpr = getOpenGL().getDevicePixelRatio();

    auto &font = getGLFont();
    std::vector<GLText> text;

    const auto lineHeight = font.getFontHeight();
    const float rightMargin = float(w) * dpr
                              - static_cast<float>(font.getGlyphAdvance('e').value_or(5));

    // x and y are in physical (device) pixels
    // TODO: change API to use logical pixels.
    auto y = lineHeight;
    const auto print = [lineHeight, rightMargin, &text, &y](const QString &msg) {
        text.emplace_back(glm::vec3(rightMargin, y, 0),
                          mmqt::toStdStringUtf8(msg),
                          Colors::white,
                          Colors::black.withAlpha(0.4f),
                          FontFormatFlags{FontFormatFlagEnum::HALIGN_RIGHT});
        y += lineHeight;
    };

    const auto texturesTime = ms(afterTextures - start);
    const auto batchTime = ms(afterBatches - afterTextures);

    const auto total = ms(end - start);
    print(QString::asprintf(
        "%.1f (updateTextures) + %.1f (updateBatches) + %.1f (paintGL) + %.1f (glFinish%s) = %.1f ms",
        texturesTime,
        batchTime,
        ms(afterPaint - afterBatches),
        ms(end - afterPaint),
        calledFinish ? "" : "*",
        total));

    if (!calledFinish) {
        print("* = unable to call glFinish()");
    }

    longestBatchMs = std::max(batchTime, longestBatchMs);
    print(QString::asprintf("Worst updateBatches: %.1f ms", longestBatchMs));

#ifdef Q_OS_WASM
    {
        // Browser memory is what limits the map on tablets; show the wasm
        // heap and the offscreen framebuffer's footprint next to the timings.
        const size_t heapSize = emscripten_get_heap_size();
        const size_t heapMax = emscripten_get_heap_max();
        const Viewport fbo = getOpenGL().getPhysicalViewport();
        const int samples = std::max(1, getConfig().canvas.antialiasingSamples.get());
        const double fboMegabytes = static_cast<double>(fbo.size.x)
                                    * static_cast<double>(fbo.size.y)
                                    * 8.0 /* RGBA8 color + 32-bit depth */
                                    * static_cast<double>(samples) / (1024.0 * 1024.0);
        print(QString::asprintf("WASM heap: %zu MB of %zu MB",
                                heapSize / (1024 * 1024),
                                heapMax / (1024 * 1024)));
        print(QString::asprintf("FBO: %dx%d at %d%% (~%.1f MB)",
                                fbo.size.x,
                                fbo.size.y,
                                getConfig().canvas.renderScale.get(),
                                fboMegabytes));
    }
#endif

    const auto &advanced = getConfig().canvas.advanced;
    const float zoom = getTotalScaleFactor();
    const bool is3d = advanced.use3D.get();
    if (is3d) {
        const ViewportConfig config{advanced.use3D.get(),
                                    advanced.autoTilt.get(),
                                    advanced.fov.getFloat(),
                                    advanced.verticalAngle.getFloat(),
                                    advanced.horizontalAngle.getFloat(),
                                    advanced.layerHeight.getFloat()};
        print(QString::asprintf("3d mode: %.1f fovy, %.1f pitch, %.1f yaw, %.1f zscale",
                                advanced.fov.getDouble(),
                                static_cast<double>(
                                    ProjectionUtils::calculatePitchDegrees(config, zoom)),
                                advanced.horizontalAngle.getDouble(),
                                advanced.layerHeight.getDouble()));
    } else {
        const glm::vec3 c = unproject_raw(glm::vec3{w / 2, h / 2, 0});
        const glm::vec3 v = unproject_raw(glm::vec3{w / 2, 0, 0});
        const auto dy = std::abs((v - c).y);
        const auto dz = std::abs(c.z);
        const float fovy = 2.f * glm::degrees(std::atan2(dy, dz));
        print(QString::asprintf("2d mode; current fovy: %.1f", static_cast<double>(fovy)));
    }

    print(QString::asprintf("zoom: %.2f (1/%.1f)",
                            static_cast<double>(zoom),
                            1.0 / static_cast<double>(zoom)));

    const auto ctr = m_mapScreen.getCenter();
    print(QString::asprintf("center: %.1f, %.1f, %.1f",
                            static_cast<double>(ctr.x),
                            static_cast<double>(ctr.y),
                            static_cast<double>(ctr.z)));

    font.render2dTextImmediate(text);
}

void MapCanvas::paintSelectionArea()
{
    if (!hasSel1() || !hasSel2()) {
        return;
    }

    const auto pos1 = getSel1().pos.to_vec2();
    const auto pos2 = getSel2().pos.to_vec2();

    // Mouse selected area
    auto &gl = getOpenGL();
    const auto layer = static_cast<float>(getCurrentLayer());

    if (hasAreaSelection()) {
        const glm::vec3 A{pos1, layer};
        const glm::vec3 B{pos2.x, pos1.y, layer};
        const glm::vec3 C{pos2, layer};
        const glm::vec3 D{pos1.x, pos2.y, layer};

        // REVISIT: why a dark colored selection?
        const Color selBgColor = Colors::black.withAlpha(0.5f);
        const auto rs
            = GLRenderState().withBlend(BlendModeEnum::TRANSPARENCY).withDepthFunction(std::nullopt);

        {
            const std::vector<glm::vec3> verts{A, B, C, D};
            const auto &fillStyle = rs;
            gl.renderPlainQuads(verts, fillStyle.withColor(selBgColor));
        }

        const auto selFgColor = Colors::yellow;
        {
            static constexpr float SELECTION_AREA_LINE_WIDTH = 2.f;
            const auto lineStyle = rs.withLineParams(LineParams{SELECTION_AREA_LINE_WIDTH});
            const std::vector<glm::vec3> verts{A, B, B, C, C, D, D, A};

            // FIXME: ASAN flags this as out-of-bounds memory access inside an assertion
            //
            //     Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
            //
            // in QOpenGLFunctions::glDrawArrays(). However, it works without ASAN,
            // so maybe the problem is in my OpenGL driver?
            //
            // "OpenGL Version:" "3.1 Mesa 20.2.6"
            // "OpenGL Renderer:" "llvmpipe (LLVM 11.0.0, 256 bits)"
            // "OpenGL Vendor:" "Mesa/X.org"
            // "OpenGL GLSL:" "1.40"
            // "Current OpenGL Context:" "3.1 (valid)"
            //
            gl.renderPlainLines(verts, lineStyle.withColor(selFgColor));
        }
    }

    paintNewInfomarkSelection();
}

void MapCanvas::updateMultisampling()
{
    // MSAA multiplies the FBO's memory, which a reduced render scale exists
    // to save, so the two are exclusive (the Graphics page enforces the same).
    const int wantMultisampling = (getConfig().canvas.renderScale.get() < 100)
                                      ? 0
                                      : getConfig().canvas.antialiasingSamples.get();
    getOpenGL().configureFbo(wantMultisampling);
}

float MapCanvas::configuredRenderScale()
{
    return std::clamp(static_cast<float>(getConfig().canvas.renderScale.get()) / 100.f, 0.25f, 1.f);
}

void MapCanvas::renderMapBatches()
{
    std::optional<MapBatches> &mapBatches = m_batches.mapBatches;
    if (!mapBatches.has_value()) {
        // Hint: Use CREATE_ONLY first.
        throw std::runtime_error("called in the wrong order");
    }

    MapBatches &batches = mapBatches.value();
    const Configuration::CanvasSettings &settings = getConfig().canvas;

    const float totalScaleFactor = getTotalScaleFactor();
    const auto wantExtraDetail = totalScaleFactor >= settings.extraDetailScaleCutoff;
    const auto wantDoorNames = settings.drawDoorNames
                               && (totalScaleFactor >= settings.doorNameScaleCutoff);

    auto &gl = getOpenGL();

    BatchedMeshes &batchedMeshes = batches.batchedMeshes;

    const auto drawLayer =
        [&batches, &batchedMeshes, wantExtraDetail, wantDoorNames](const int thisLayer,
                                                                   const int currentLayer) {
            const auto it_mesh = batchedMeshes.find(thisLayer);
            if (it_mesh != batchedMeshes.end()) {
                LayerMeshes &meshes = it_mesh->second;
                meshes.render(thisLayer, currentLayer);
            }

            if (wantExtraDetail) {
                BatchedConnectionMeshes &connectionMeshes = batches.connectionMeshes;
                const auto it_conn = connectionMeshes.find(thisLayer);
                if (it_conn != connectionMeshes.end()) {
                    ConnectionMeshes &meshes = it_conn->second;
                    meshes.render(thisLayer, currentLayer);
                }

                // NOTE: This can display room names in lower layers, but the text
                // isn't currently drawn with an appropriate Z-offset, so it doesn't
                // stay aligned to its actual layer when you switch view layers.
                if (wantDoorNames && thisLayer == currentLayer) {
                    BatchedRoomNames &roomNameBatches = batches.roomNameBatches;
                    const auto it_name = roomNameBatches.find(thisLayer);
                    if (it_name != roomNameBatches.end()) {
                        auto &roomNameBatch = it_name->second;
                        roomNameBatch.render(GLRenderState());
                    }
                }
            }
        };

    const auto fadeBackground = [&gl, &settings]() {
        auto bgColor = Color{settings.backgroundColor.getColor(), 0.5f};

        const auto blendedWithBackground
            = GLRenderState().withBlend(BlendModeEnum::TRANSPARENCY).withColor(bgColor);

        gl.renderPlainFullScreenQuad(blendedWithBackground);
    };

    const int currentLayer = getCurrentLayer();
    for (const auto &layer : batchedMeshes) {
        const int thisLayer = layer.first;
        if (thisLayer == currentLayer) {
            gl.clearDepth();
            fadeBackground();
        }
        drawLayer(thisLayer, currentLayer);
    }
}
