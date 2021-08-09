// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "mapcanvas.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QOpenGLWidget>
#include <QtCore>
#include <QtGui/qopengl.h>
#include <QtGui>

#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../global/Array.h"
#include "../global/ChangeMonitor.h"
#include "../global/Debug.h"
#include "../global/RuleOf5.h"
#include "../global/utils.h"
#include "../mapdata/mapdata.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "Connections.h"
#include "MapCanvasConfig.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "Textures.h"
#include "connectionselection.h"

namespace MapCanvasConfig {

static std::mutex g_version_lock;
static std::string g_current_gl_version = "UN0.0";

static void setCurrentOpenGLVersion(std::string version)
{
    std::unique_lock<std::mutex> lock{g_version_lock};
    g_current_gl_version = std::move(version);
}

std::string getCurrentOpenGLVersion()
{
    std::unique_lock<std::mutex> lock{g_version_lock};
    return g_current_gl_version;
}

ConnectionSet registerChangeCallback(ChangeMonitor::Function callback)
{
    return setConfig().canvas.advanced.registerChangeCallback(std::move(callback));
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

class NODISCARD MakeCurrentRaii final
{
private:
    QOpenGLWidget &m_glWidget;

public:
    explicit MakeCurrentRaii(QOpenGLWidget &widget)
        : m_glWidget{widget}
    {
        m_glWidget.makeCurrent();
    }
    ~MakeCurrentRaii() { m_glWidget.doneCurrent(); }

    DELETE_CTORS_AND_ASSIGN_OPS(MakeCurrentRaii);
};

void MapCanvas::cleanupOpenGL()
{
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    MakeCurrentRaii makeCurrentRaii{*this};

    // note: m_batchedMeshes co-owns textures created by MapCanvasData,
    // and it also owns the lifetime of some OpenGL objects (e.g. VBOs).
    m_batches.resetAll();
    m_textures.destroyAll();
    getGLFont().cleanup();
    getOpenGL().cleanup();
    m_logger.reset();
}

/// Must be called from constructor;
/// initializeGL() will fail if you forget to call this.
void MapCanvas::initSurface()
{
    const auto getAaSamples = []() {
        int samples = getConfig().canvas.antialiasingSamples;
        if (samples <= 0) {
            samples = 2; // Default to 2 samples to prevent restart
        }
        return samples;
    };
    const int samples = getAaSamples();
    QSurfaceFormat format;
    format.setSamples(samples);
    setFormat(format);
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

    logString("OpenGL Version:", GL_VERSION);
    logString("OpenGL Renderer:", GL_RENDERER);
    logString("OpenGL Vendor:", GL_VENDOR);
    logString("OpenGL GLSL:", GL_SHADING_LANGUAGE_VERSION);

    const auto version = [this]() -> std::string {
        const QSurfaceFormat &format = context()->format();
        std::ostringstream oss;
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
        return std::move(oss).str();
    }();

    MapCanvasConfig::setCurrentOpenGLVersion(version);

    logMsg("Current OpenGL Context:",
           QString("%1 (%2)")
               .arg(version.c_str())
               // FIXME: This is a bit late to report an invalid context.
               .arg(context()->isValid() ? "valid" : "invalid")
               .toUtf8());
    logMsg("Display:", QString("%1 DPI").arg(QPaintDevice::devicePixelRatioF()).toUtf8());
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

void MapCanvas::initializeGL()
{
    auto &gl = getOpenGL();
    gl.initializeOpenGLFunctions();

    reportGLVersion();

    // TODO: Perform the blacklist test as a call from main() to minimize player headache.
    if (isBlacklistedDriver()) {
        setConfig().canvas.softwareOpenGL = true;
        setConfig().write();
        hide();
        doneCurrent();
        QMessageBox::critical(this,
                              "OpenGL Driver Blacklisted",
                              "Please restart MMapper to enable software rendering");
        return;
    }

    // NOTE: If you're adding code that relies on generating OpenGL errors (e.g. ANGLE),
    // you *MUST* force it to complete those error probes before calling initLogger(),
    // because the logger purposely calls std::abort() when it receives an error.
    initLogger();

    gl.initializeRenderer(static_cast<float>(QPaintDevice::devicePixelRatioF()));
    updateMultisampling();
    initTextures();
    getGLFont().init();
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
    std::abort();
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

glm::mat4 MapCanvas::getViewProj_old(const glm::vec2 &scrollPos,
                                     const glm::ivec2 &size,
                                     const float zoomScale,
                                     const int /*currentLayer*/)
{
    constexpr float FIXED_VIEW_DISTANCE = 60.f;
    constexpr float ROOM_Z_SCALE = 7.f;
    constexpr auto baseSize = static_cast<float>(BASESIZE);

    const float swp = zoomScale * baseSize / static_cast<float>(size.x);
    const float shp = zoomScale * baseSize / static_cast<float>(size.y);

    QMatrix4x4 proj;
    proj.frustum(-0.5f, +0.5f, -0.5f, 0.5f, 5.f, 10000.f);
    proj.scale(swp, shp, 1.f);
    proj.translate(-scrollPos.x, -scrollPos.y, -FIXED_VIEW_DISTANCE);
    proj.scale(1.f, 1.f, ROOM_Z_SCALE);

    return glm::make_mat4(proj.constData());
}

NODISCARD static float getPitchDegrees(const float zoomScale)
{
    const float degrees = getConfig().canvas.advanced.verticalAngle.getFloat();
    if (!MapCanvasConfig::isAutoTilt())
        return degrees;

    static_assert(ScaleFactor::MAX_VALUE >= 2.f);
    return glm::smoothstep(0.5f, 2.f, zoomScale) * degrees;
}

glm::mat4 MapCanvas::getViewProj(const glm::vec2 &scrollPos,
                                 const glm::ivec2 &size,
                                 const float zoomScale,
                                 const int currentLayer)
{
    static_assert(GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_RH_NO);

    const int width = size.x;
    const int height = size.y;

    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    const auto &advanced = getConfig().canvas.advanced;
    const float fovDegrees = advanced.fov.getFloat();
    const auto pitchRadians = glm::radians(getPitchDegrees(zoomScale));
    const auto yawRadians = glm::radians(advanced.horizontalAngle.getFloat());
    const auto layerHeight = advanced.layerHeight.getFloat();

    const auto pixelScale = [aspect, fovDegrees, width]() -> float {
        constexpr float HARDCODED_LOGICAL_PIXELS = 44.f;
        const auto dummyProj = glm::perspective(glm::radians(fovDegrees), aspect, 1.f, 10.f);

        const auto centerRoomProj = glm::inverse(dummyProj) * glm::vec4(0.f, 0.f, 0.f, 1.f);
        const auto centerRoom = glm::vec3(centerRoomProj) / centerRoomProj.w;

        // Use east instead of north, so that tilted perspective matches horizontally.
        const auto oneRoomEast = dummyProj * glm::vec4(centerRoom + glm::vec3(1.f, 0.f, 0.f), 1.f);
        const float clipDist = std::abs(oneRoomEast.x / oneRoomEast.w);
        const float ndcDist = clipDist * 0.5f;

        // width is in logical pixels
        const float screenDist = ndcDist * static_cast<float>(width);
        const auto pixels = std::abs(centerRoom.z) * screenDist;
        return pixels / HARDCODED_LOGICAL_PIXELS;
    }();

    const float ZSCALE = layerHeight;
    const float camDistance = pixelScale / zoomScale;
    const auto center = glm::vec3(scrollPos, static_cast<float>(currentLayer) * ZSCALE);

    // The view matrix will transform from world space to eye-space.
    // Eye space has the camera at the origin, with +X right, +Y up, and -Z forward.
    //
    // Our camera's orientation is based on the world-space ENU coordinates.
    // We'll define right-handed basis vectors forward, right, and up.

    // The horizontal rotation in the XY plane will affect both forward and right vectors.
    // Currently the convention is: -45 is northwest, and +45 is northeast.
    //
    // If you want to modify this, keep in mind that the angle is inverted since the
    // camera is subtracted from the center, so the result is that positive angle
    // appears clockwise (backwards) on screen.
    const auto rotateHorizontal = glm::mat3(
        glm::rotate(glm::mat4(1), -yawRadians, glm::vec3(0, 0, 1)));

    // Our unrotated pitch is defined so that 0 is straight down, and 90 degrees is north,
    // but the yaw rotation can cause it to point northeast or northwest.
    //
    // Here we use an ENU coordinate system, so we have:
    //   forward(pitch= 0 degrees) = -Z (down), and
    //   forward(pitch=90 degrees) = +Y (north).
    const auto forward = rotateHorizontal
                         * glm::vec3(0.f, std::sin(pitchRadians), -std::cos(pitchRadians));
    // Unrotated right is east (+X).
    const auto right = rotateHorizontal * glm::vec3(1, 0, 0);
    // right x forward = up
    const auto up = glm::cross(right, glm::normalize(forward));

    // Subtract because camera looks at the center.
    const auto eye = center - camDistance * forward;

    // NOTE: may need to modify near and far planes by pixelScale and zoomScale.
    // Be aware that a 24-bit depth buffer only gives about 12 bits of usable
    // depth range; we may need to reduce this for people with 16-bit depth buffers.
    // Keep in mind: Arda is about 600x300 rooms, so viewing the blue mountains
    // from mordor requires approx 700 room units of view distance.
    const auto proj = glm::perspective(glm::radians(fovDegrees), aspect, 0.25f, 1024.f);
    const auto view = glm::lookAt(eye, center, up);
    const auto scaleZ = glm::scale(glm::mat4(1), glm::vec3(1.f, 1.f, ZSCALE));

    return proj * view * scaleZ;
}

void MapCanvas::setMvp(const glm::mat4 &viewProj)
{
    auto &gl = getOpenGL();
    m_viewProj = viewProj;
    gl.setProjectionMatrix(m_viewProj);
}

void MapCanvas::setViewportAndMvp(int width, int height)
{
    const bool want3D = getConfig().canvas.advanced.use3D.get();

    auto &gl = getOpenGL();

    gl.glViewport(0, 0, width, height);
    const auto size = getViewport().size;
    assert(size.x == width);
    assert(size.y == height);

    const float zoomScale = getTotalScaleFactor();
    const auto viewProj = (!want3D) ? getViewProj_old(m_scroll, size, zoomScale, m_currentLayer)
                                    : getViewProj(m_scroll, size, zoomScale, m_currentLayer);
    setMvp(viewProj);
}

void MapCanvas::resizeGL(int width, int height)
{
    if (m_textures.update == nullptr) {
        // resizeGL called but initializeGL was not called yet
        return;
    }

    setViewportAndMvp(width, height);

    // Render
    update();
}

void MapCanvas::updateBatches()
{
    updateMapBatches();
    updateInfomarkBatches();
}

void MapCanvas::updateMapBatches()
{
    if (m_data.getNeedsMapUpdate()) {
        m_batches.mapBatches.reset();
        m_data.clearNeedsMapUpdate();
        assert(!m_data.getNeedsMapUpdate());
    }

    const Coordinate &center = [this]() {
        const auto &screenCenter = m_mapScreen.getCenter();
        return Coordinate{static_cast<int>(screenCenter.x),
                          static_cast<int>(screenCenter.y),
                          m_currentLayer};
    }();
    std::optional<MapBatches> &opt_mapBatches = m_batches.mapBatches;
    if (opt_mapBatches && opt_mapBatches->redrawMargin.contains(center)) {
        return;
    }

    const auto radius = []() -> Coordinate {
        const auto &r = getConfig().canvas.mapRadius;
        return Coordinate{r[0], r[1], r[2]};
    }();

    const OptBounds bounds = [&center, &radius]() -> OptBounds {
        // TODO: allow unrestricted map if there hasn't been a map update or movement within N seconds.
        // This could be done by using a timer to increment a counter on mapBatches,
        // and then reset the counter if the player moves.
        const bool restrict = []() -> bool {
            const Configuration &config = getConfig();
            switch (config.canvas.useRestrictedMap) {
            case RestrictMapEnum::Never:
                return false;
            case RestrictMapEnum::Always:
                return true;
            case RestrictMapEnum::OnlyInMapMode:
                return config.general.mapMode == MapModeEnum::MAP;
            }
            std::abort();
        }();

        return restrict                                          //
                   ? OptBounds::fromCenterRadius(center, radius) //
                   : OptBounds{};                                //
    }();

    MapCanvasRoomDrawer drawer{m_textures, getOpenGL(), getGLFont(), opt_mapBatches};

    /// The following ends up calling MapCanvasRoomDrawer::generateBatches()
    m_data.generateBatches(drawer, bounds);

    if (bounds.isRestricted()) {
        m_batches.mapBatches->redrawMargin = OptBounds::fromCenterRadius(center, radius * 3 / 4);
    }
}

void MapCanvas::actuallyPaintGL()
{
    setViewportAndMvp(width(), height());

    auto &gl = getOpenGL();
    gl.clear(Color{getConfig().canvas.backgroundColor});

    if (m_data.isEmpty()) {
        getGLFont().renderTextCentered("No map loaded");
        return;
    }

    paintMap();
    paintBatchedInfomarks();
    paintSelections();
    paintCharacters();
}

void MapCanvas::paintMap()
{
    if (!m_batches.mapBatches.has_value()) {
        // This should never happen.
        getGLFont().renderTextCentered("Batch error");
        return;
    }

    renderMapBatches();
}

void MapCanvas::paintSelections()
{
    paintSelectedRooms();
    paintSelectedConnection();
    paintSelectionArea();
    paintSelectedInfoMarks();
}

void MapCanvas::paintGL()
{
    static thread_local double longestBatchMs = 0.0;

    const bool showPerfStats = MapCanvasConfig::getShowPerfStats();

    using Clock = std::chrono::high_resolution_clock;
    std::optional<Clock::time_point> optStart;
    std::optional<Clock::time_point> optAfterTextures;
    std::optional<Clock::time_point> optAfterBatches;
    if (showPerfStats)
        optStart = Clock::now();

    {
        updateMultisampling();
        updateTextures();
        if (showPerfStats)
            optAfterTextures = Clock::now();

        // Note: The real work happens here!
        updateBatches();

        // For accurate timing of the update, we'd need to call glFinish(),
        // or at least set up an OpenGL query object. The update will send
        // a lot of data to the GPU, so it could take a while...
        if (showPerfStats)
            optAfterBatches = Clock::now();

        actuallyPaintGL();
    }

    if (!showPerfStats)
        return; /* don't wait to finish */

    const auto &start = optStart.value();
    const auto &afterTextures = optAfterTextures.value();
    const auto &afterBatches = optAfterBatches.value();
    const auto afterPaint = Clock::now();
    const bool calledFinish = [this]() -> bool {
        if (auto *const ctxt = QOpenGLWidget::context())
            if (auto *const func = ctxt->functions()) {
                func->glFinish();
                return true;
            }
        return false;
    }();

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
                          ::toStdStringLatin1(msg),
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
    if (!calledFinish)
        print("* = unable to call glFinish()");

    longestBatchMs = std::max(batchTime, longestBatchMs);
    print(QString::asprintf("Worst updateBatches: %.1f ms", longestBatchMs));

    const auto &advanced = getConfig().canvas.advanced;
    const float zoom = getTotalScaleFactor();
    const bool is3d = advanced.use3D.get();
    if (is3d) {
        print(QString::asprintf("3d mode: %.1f fovy, %.1f pitch, %.1f yaw, %.1f zscale",
                                advanced.fov.getDouble(),
                                static_cast<double>(getPitchDegrees(zoom)),
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
    if (!hasSel1() || !hasSel2())
        return;

    const auto pos1 = getSel1().pos.to_vec2();
    const auto pos2 = getSel2().pos.to_vec2();

    // Mouse selected area
    auto &gl = getOpenGL();
    const auto layer = static_cast<float>(m_currentLayer);

    if (m_selectedArea) {
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
            gl.renderPlainLines(verts, lineStyle.withColor(selFgColor));
        }
    }

    paintNewInfomarkSelection();
}

void MapCanvas::updateMultisampling()
{
    const int wantMultisampling = getConfig().canvas.antialiasingSamples;
    std::optional<int> &activeStatus = graphicsOptionsStatus.multisampling;
    if (activeStatus == wantMultisampling)
        return;

    // REVISIT: check return value?
    MAYBE_UNUSED const bool enabled = getOpenGL().tryEnableMultisampling(wantMultisampling);
    activeStatus = wantMultisampling;
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
                {
                    BatchedConnectionMeshes &connectionMeshes = batches.connectionMeshes;
                    const auto it_conn = connectionMeshes.find(thisLayer);
                    if (it_conn != connectionMeshes.end()) {
                        ConnectionMeshes &meshes = it_conn->second;
                        meshes.render(thisLayer, currentLayer);
                    }
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

    for (const auto &layer : batchedMeshes) {
        const int thisLayer = layer.first;
        if (thisLayer == m_currentLayer) {
            gl.clearDepth();
            fadeBackground();
        }
        drawLayer(thisLayer, m_currentLayer);
    }
}
