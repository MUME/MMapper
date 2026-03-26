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

out vec4 vFragmentColor;

void main()
{
    float uCurrentTime = uTime.x;
    float uTimeOfDayStartTime = uConfig.y;
    float uTransitionDuration = uConfig.z;

    vec4 timeOfDayStart = uNamedColors[int(uTimeOfDay.x)];
    vec4 timeOfDayTarget = uNamedColors[int(uTimeOfDay.y)];

    float timeOfDayLerp = clamp((uCurrentTime - uTimeOfDayStartTime) / uTransitionDuration,
                                0.0,
                                1.0);
    float currentTimeOfDayIntensity = mix(uTimeOfDay.z, uTimeOfDay.w, timeOfDayLerp);

    vec4 uTimeOfDayColor = mix(timeOfDayStart, timeOfDayTarget, timeOfDayLerp);
    uTimeOfDayColor.a *= currentTimeOfDayIntensity;

    vFragmentColor = uTimeOfDayColor;
}
