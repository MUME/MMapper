#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../clock/mumemoment.h"
#include "../global/ChangeMonitor.h"
#include "../global/ConfigEnums.h"
#include "../global/RuleOf5.h"
#include "../map/PromptFlags.h"
#include "../map/coordinate.h"
#include "../observer/gameobserver.h"
#include "OpenGL.h"

class FrameManager;
#include "OpenGLTypes.h"
#include "legacy/Legacy.h"
#include "legacy/WeatherMeshes.h"

#include <chrono>

#undef TRANSPARENT // Bad dog, Microsoft; bad dog!!!
#include <memory>

class MapData;
struct MapCanvasTextures;

namespace WeatherConstants {
/**
 * @brief Spatial parameters for weather particles and atmosphere.
 *
 * WEATHER_RADIUS (14.0) defines the world-space distance from the player
 * where particles wrap and atmosphere extents are calculated.
 *
 * WEATHER_EXTENT (28.0) is the total diameter of the toroidal simulation area.
 *
 * Atmosphere and particle masks typically fade out at a 12.0 radius to avoid
 * sharp edges at the simulation boundaries.
 */
constexpr float WEATHER_RADIUS = 14.0f;
constexpr float WEATHER_EXTENT = 28.0f;
constexpr float WEATHER_MASK_RADIUS_OUTER = 12.0f;
constexpr float WEATHER_MASK_RADIUS_INNER = 8.0f;
constexpr float TRANSITION_DURATION = 2.0f;
} // namespace WeatherConstants

/**
 * @brief Manages the logic and rendering of the weather system.
 * Follows the design and structure of GLFont.
 */
class NODISCARD GLWeather final
{
private:
    struct FrameManagerProxy;
    OpenGL &m_gl;
    MapData &m_data;
    const MapCanvasTextures &m_textures;
    FrameManager &m_animationManager;
    GameObserver &m_observer;
    ChangeMonitor::Lifetime m_lifetime;

    // Weather State
    float m_currentRainIntensity = 0.0f;
    float m_currentSnowIntensity = 0.0f;
    float m_currentCloudsIntensity = 0.0f;
    float m_currentFogIntensity = 0.0f;
    float m_currentTimeOfDayIntensity = 0.0f;

    float m_gameRainIntensity = 0.0f;
    float m_gameSnowIntensity = 0.0f;
    float m_gameCloudsIntensity = 0.0f;
    float m_gameFogIntensity = 0.0f;
    float m_gameTimeOfDayIntensity = 0.0f;
    MumeTimeEnum m_currentTimeOfDay = MumeTimeEnum::DAY;
    MumeMoonVisibilityEnum m_moonVisibility = MumeMoonVisibilityEnum::UNKNOWN;

    // Meshes
    std::unique_ptr<Legacy::ParticleSimulationMesh> m_simulation;
    std::unique_ptr<Legacy::ParticleRenderMesh> m_particles;
    UniqueMesh m_atmosphere;
    UniqueMesh m_timeOfDay;

public:
    explicit GLWeather(OpenGL &gl,
                       MapData &mapData,
                       const MapCanvasTextures &textures,
                       GameObserver &observer,
                       FrameManager &frameManager);
    ~GLWeather();

    DELETE_CTORS_AND_ASSIGN_OPS(GLWeather);

public:
    void update();
    void prepare();
    void render(const GLRenderState &rs);

    NODISCARD bool isAnimating() const;
    NODISCARD bool isTransitioning() const;

private:
    void updateTargets();
    void updateFromGame();
    void initMeshes();
    void syncWeatherAtmosphere();
    void syncWeatherTimeOfDay();

    NODISCARD float applyTransition(float startTime, float startVal, float targetVal) const;

    template<typename T>
    NODISCARD T applyTransition(float startTime, T startVal, T targetVal) const;

    template<typename T>
    struct NODISCARD TransitionPair final
    {
        T &start;
        T target;
    };

    template<typename... Pairs>
    void startTransitions(float &startTime, Pairs... pairs);

    NODISCARD NamedColorEnum getCurrentColorIdx() const;
};
