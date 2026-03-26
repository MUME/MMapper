// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

layout(location = 0) in vec2 aParticlePos;
layout(location = 1) in float aLife;

layout(std140) uniform CameraBlock
{
    mat4 uViewProj;
    vec4 uPlayerPos;        // xyz, w=zScale
};

layout(std140) uniform TimeBlock
{
    vec4 uTime; // x=time, y=delta, zw=unused
};

layout(std140) uniform WeatherBlock
{
    vec4 uIntensities; // rain_start, snow_start, clouds_start, fog_start
    vec4 uTargets;     // rain_target, snow_target, clouds_target, fog_target
    vec4 uTimeOfDay;   // x=startIdx, y=targetIdx, z=todStart, w=todTarget
    vec4 uConfig;      // x=weatherStartTime, y=todStartTime, z=duration, w=unused
};

out float vLife;
out vec2 vLocalCoord;
out float vLocalMask;

float rand(float n)
{
    return fract(sin(n) * 43758.5453123);
}

void main()
{
    float uCurrentTime = uTime.x;
    float uZScale = uPlayerPos.w;
    float uWeatherStartTime = uConfig.x;
    float uTransitionDuration = uConfig.z;

    float weatherLerp = clamp((uCurrentTime - uWeatherStartTime) / uTransitionDuration, 0.0, 1.0);
    float pRain = mix(uIntensities.x, uTargets.x, weatherLerp);
    float pSnow = mix(uIntensities.y, uTargets.y, weatherLerp);
    float pIntensity = max(pRain, pSnow);
    float pType = pSnow / max(pIntensity, 0.001);

    float hash = rand(float(gl_InstanceID));

    // Generate quad position from gl_VertexID (0..3)
    vec2 quadPos = vec2(float(gl_VertexID & 1) - 0.5, float((gl_VertexID >> 1) & 1) - 0.5);

    vLife = aLife;
    vLocalCoord = quadPos + 0.5;

    // Rain size
    vec2 rainSize = vec2(1.0 / 12.0, 1.0 / (0.25 - clamp(pIntensity, 0.0, 2.0) * 0.1));
    // Snow size
    vec2 snowSize = vec2(1.0 / 4.0, 1.0 / 4.0);

    vec2 size = mix(rainSize, snowSize, pType);
    vec2 pos = aParticlePos;

    // Extra swaying for snow visuals
    pos.x += sin(uCurrentTime * 1.2 + hash * 6.28) * 0.4 * pType;

    vec3 worldPos = vec3(pos + quadPos * size, uPlayerPos.z);

    float distToPlayer = distance(pos, uPlayerPos.xy);
    vLocalMask = 1.0 - smoothstep(WEATHER_MASK_RADIUS_INNER, WEATHER_MASK_RADIUS_OUTER, distToPlayer);

    gl_Position = uViewProj * vec4(worldPos.x, worldPos.y, worldPos.z * uZScale, 1.0);
}
