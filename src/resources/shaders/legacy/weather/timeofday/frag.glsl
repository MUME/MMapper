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

vec3 srgbToLinear(vec3 c)
{
    vec3 low = c / 12.92;
    vec3 high = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(low, high, step(vec3(0.04045), c));
}

vec3 linearToSrgb(vec3 c)
{
    vec3 low = c * 12.92;
    vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    return mix(low, high, step(vec3(0.0031308), c));
}

vec3 linearToOklab(vec3 c)
{
    float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
    float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
    float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;

    float l_ = pow(l, 1.0 / 3.0);
    float m_ = pow(m, 1.0 / 3.0);
    float s_ = pow(s, 1.0 / 3.0);

    return vec3(0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720403 * s_,
                1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
                0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_);
}

vec3 oklabToLinear(vec3 c)
{
    float l_ = c.x + 0.3963377774 * c.y + 0.2158037573 * c.z;
    float m_ = c.x - 0.1055613458 * c.y - 0.0638541728 * c.z;
    float s_ = c.x - 0.0894841775 * c.y - 1.2914855480 * c.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return vec3(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
                -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
                -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s);
}

void main()
{
    float uCurrentTime = uTime.x;
    float uTimeOfDayStartTime = uConfig.y;
    float uTransitionDuration = uConfig.z;

    vec4 timeOfDayStartSrgb = uNamedColors[int(uTimeOfDay.x)];
    vec4 timeOfDayTargetSrgb = uNamedColors[int(uTimeOfDay.y)];

    float t = clamp((uCurrentTime - uTimeOfDayStartTime) / uTransitionDuration, 0.0, 1.0);
    float timeOfDayLerp = smoothstep(0.0, 1.0, t);

    // Convert start and target colors to Oklab
    vec3 startOklab = linearToOklab(srgbToLinear(timeOfDayStartSrgb.rgb));
    vec3 targetOklab = linearToOklab(srgbToLinear(timeOfDayTargetSrgb.rgb));

    // Interpolate in Oklab space
    vec3 mixedOklab = mix(startOklab, targetOklab, timeOfDayLerp);

    // Convert back to sRGB
    vec3 mixedSrgb = linearToSrgb(oklabToLinear(mixedOklab));

    // Mix intensity with smoothstep as well
    float currentTimeOfDayIntensity = mix(uTimeOfDay.z, uTimeOfDay.w, timeOfDayLerp);

    // Final color with alpha
    float finalAlpha = mix(timeOfDayStartSrgb.a, timeOfDayTargetSrgb.a, timeOfDayLerp);
    finalAlpha *= currentTimeOfDayIntensity;

    vFragmentColor = vec4(mixedSrgb, finalAlpha);
}
