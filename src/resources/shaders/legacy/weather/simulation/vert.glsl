// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

layout(location = 0) in vec2 aPos;
layout(location = 1) in float aLife;

layout(std140) uniform CameraBlock
{
    mat4 uViewProj;
    vec4 uPlayerPos;   // xyz, w=zScale
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

out vec2 vPos;
out float vLife;

float hash21(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float rand(float n)
{
    return fract(sin(n) * 43758.5453123);
}

void main()
{
    float hash = rand(float(gl_VertexID));
    float uCurrentTime = uTime.x;
    float uDeltaTime = uTime.y;
    float uWeatherStartTime = uConfig.x;
    float uTransitionDuration = uConfig.z;

    float weatherLerp = clamp((uCurrentTime - uWeatherStartTime) / uTransitionDuration, 0.0, 1.0);
    float pRain = mix(uIntensities.x, uTargets.x, weatherLerp);
    float pSnow = mix(uIntensities.y, uTargets.y, weatherLerp);
    float pIntensity = max(pRain, pSnow);
    float pType = pSnow / max(pIntensity, 0.001); // 0=rain, 1=snow

    vec2 pos = aPos;
    float life = aLife;

    // Rain physics
    float rainSpeed = (15.0 + pIntensity * 10.0) + hash * 5.0;
    float rainDecay = 0.35 + hash * 0.15;

    // Snow physics
    float snowSpeed = (0.75 + pIntensity * 0.75) + hash * 0.5;
    float snowDecay = 0.015 + hash * 0.01;

    // Interpolate physics based on pType
    float speed = mix(rainSpeed, snowSpeed, pType);
    float decay = mix(rainDecay, snowDecay, pType);

    pos.y -= uDeltaTime * speed;

    // Horizontal swaying (only for snow)
    pos.x += sin(uCurrentTime * 1.2 + hash * 6.28) * (0.1 + pIntensity * 0.2) * pType * uDeltaTime;

    life -= uDeltaTime * decay;

    // Spatial fading
    float hole = hash21(floor(pos.xy * 4.0));
    if (hole < 0.02) {
        life -= uDeltaTime * 2.0; // Expire quickly but not instantly
    }

    if (life <= 0.0) {
        life = 1.0;
        // Respawn at the top
        float r1 = rand(hash + uCurrentTime);
        pos.x = uPlayerPos.x + (r1 * WEATHER_EXTENT - WEATHER_RADIUS);
        pos.y = uPlayerPos.y + WEATHER_RADIUS;
    } else {
        // Toroidal wrap around player (WEATHER_EXTENT x WEATHER_EXTENT area)
        vec2 rel = pos - uPlayerPos.xy;
        if (rel.x < -WEATHER_RADIUS)
            pos.x += WEATHER_EXTENT;
        else if (rel.x > WEATHER_RADIUS)
            pos.x -= WEATHER_EXTENT;

        if (rel.y < -WEATHER_RADIUS)
            pos.y += WEATHER_EXTENT;
        else if (rel.y > WEATHER_RADIUS)
            pos.y -= WEATHER_EXTENT;
    }

    vPos = pos;
    vLife = life;
    gl_Position = vec4(vPos, 0.0, 1.0);
}
