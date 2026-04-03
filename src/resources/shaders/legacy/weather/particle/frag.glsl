// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

precision highp float;

layout(std140) uniform NamedColorsBlock
{
    vec4 uNamedColors[MAX_NAMED_COLORS];
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

in float vLife;
in vec2 vLocalCoord;
in float vLocalMask;

out vec4 vFragmentColor;

void main()
{
    float uCurrentTime = uTime.x;
    float uWeatherStartTime = uConfig.x;
    float uTimeOfDayStartTime = uConfig.y;
    float uTransitionDuration = uConfig.z;

    float weatherLerp = clamp((uCurrentTime - uWeatherStartTime) / uTransitionDuration, 0.0, 1.0);
    float pRain = mix(uIntensities.x, uTargets.x, weatherLerp);
    float pSnow = mix(uIntensities.y, uTargets.y, weatherLerp);
    float pIntensity = max(pRain, pSnow);
    float pType = pSnow / max(pIntensity, 0.001);

    float tTod = clamp((uCurrentTime - uTimeOfDayStartTime) / uTransitionDuration, 0.0, 1.0);
    float timeOfDayLerp = smoothstep(0.0, 1.0, tTod);
    float currentTimeOfDayIntensity = mix(uTimeOfDay.z, uTimeOfDay.w, timeOfDayLerp);
    float alphaStart = uNamedColors[int(uTimeOfDay.x)].a;
    float alphaTarget = uNamedColors[int(uTimeOfDay.y)].a;
    float uTimeOfDayAlpha = mix(alphaStart, alphaTarget, timeOfDayLerp) * currentTimeOfDayIntensity;

    float lifeFade = smoothstep(0.0, 0.15, vLife) * smoothstep(1.0, 0.85, vLife);

    // Rain visuals
    float streak = 1.0 - smoothstep(0.0, 0.15, abs(vLocalCoord.x - 0.5));
    float rainAlpha = mix(0.4, 0.7, clamp(pIntensity, 0.0, 1.0));
    vec4 rainColor = vec4(0.6, 0.6, 1.0, pIntensity * streak * vLocalMask * rainAlpha * lifeFade);
    rainColor.rgb += uTimeOfDayAlpha * 0.2;

    // Snow visuals
    float dist = distance(vLocalCoord, vec2(0.5));
    float flake = 1.0 - smoothstep(0.1, 0.2, dist);
    float snowAlpha = mix(0.6, 0.9, clamp(pIntensity, 0.0, 1.0));
    vec4 snowColor = vec4(1.0, 1.0, 1.1, pIntensity * flake * vLocalMask * snowAlpha * lifeFade);
    snowColor.rgb += uTimeOfDayAlpha * 0.3;

    // Interpolate visuals based on pType
    vec4 pColor = mix(rainColor, snowColor, pType);

    if (pColor.a <= 0.0) {
        discard;
    }

    vFragmentColor = pColor;
}
