// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

precision highp float;

layout(std140) uniform NamedColorsBlock
{
    vec4 uNamedColors[MAX_NAMED_COLORS];
};

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

uniform sampler2D uTexture;

in vec3 vWorldPos;
out vec4 vFragmentColor;

float get_noise(vec2 p)
{
    vec2 size = vec2(textureSize(uTexture, 0));
    return texture(uTexture, p / size).r;
}

float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    for (int i = 0; i < 4; ++i) {
        v += a * get_noise(p);
        p = p * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main()
{
    vec3 worldPos = vWorldPos;

    float distToPlayer = distance(worldPos.xy, uPlayerPos.xy);
    float localMask = 1.0 - smoothstep(WEATHER_MASK_RADIUS_INNER, WEATHER_MASK_RADIUS_OUTER, distToPlayer);

    float uCurrentTime = uTime.x;
    float uWeatherStartTime = uConfig.x;
    float uTimeOfDayStartTime = uConfig.y;
    float uTransitionDuration = uConfig.z;

    float weatherLerp = clamp((uCurrentTime - uWeatherStartTime) / uTransitionDuration, 0.0, 1.0);
    float uCloudsIntensity = mix(uIntensities[2], uTargets[2], weatherLerp);
    float uFogIntensity = mix(uIntensities[3], uTargets[3], weatherLerp);

    float timeOfDayLerp = clamp((uCurrentTime - uTimeOfDayStartTime) / uTransitionDuration,
                                0.0,
                                1.0);
    float currentTimeOfDayIntensity = mix(uTimeOfDay.z, uTimeOfDay.w, timeOfDayLerp);
    vec4 timeOfDayStart = uNamedColors[int(uTimeOfDay.x)];
    vec4 timeOfDayTarget = uNamedColors[int(uTimeOfDay.y)];
    vec4 uTimeOfDayColor = mix(timeOfDayStart, timeOfDayTarget, timeOfDayLerp);
    uTimeOfDayColor.a *= currentTimeOfDayIntensity;

    // Atmosphere overlay is now transparent by default (TimeOfDay is drawn separately)
    vec4 result = vec4(0.0);

    // Fog: soft drifting noise
    if (uFogIntensity > 0.0) {
        float n = fbm(worldPos.xy * 0.15 + uCurrentTime * 0.1);
        // Density increases non-linearly with intensity
        float density = 0.4 + uFogIntensity * 0.4;
        vec4 fog = vec4(0.8, 0.8, 0.85, uFogIntensity * n * localMask * density);
        // Emissive boost at night
        fog.rgb += uTimeOfDayColor.a * 0.15;

        // Blend fog over result
        float combinedAlpha = 1.0 - (1.0 - result.a) * (1.0 - fog.a);
        result.rgb = (result.rgb * result.a * (1.0 - fog.a) + fog.rgb * fog.a)
                     / max(combinedAlpha, 0.001);
        result.a = combinedAlpha;
    }

    // Clouds: puffy high-contrast noise
    if (uCloudsIntensity > 0.0) {
        float n = fbm(worldPos.xy * 0.06 - uCurrentTime * 0.03);
        float puffy = smoothstep(0.45, 0.55, n);
        // Clouds get darker and more "stormy" as intensity increases
        float storminess = 1.0 - uCloudsIntensity * 0.4;
        vec4 clouds = vec4(0.9 * storminess,
                           0.9 * storminess,
                           1.0 * storminess,
                           uCloudsIntensity * puffy * localMask * 0.5);
        // Emissive boost at night
        clouds.rgb += uTimeOfDayColor.a * 0.1;

        // Blend clouds over result
        float combinedAlpha = 1.0 - (1.0 - result.a) * (1.0 - clouds.a);
        result.rgb = (result.rgb * result.a * (1.0 - clouds.a) + clouds.rgb * clouds.a)
                     / max(combinedAlpha, 0.001);
        result.a = combinedAlpha;
    }

    vFragmentColor = result;
}
