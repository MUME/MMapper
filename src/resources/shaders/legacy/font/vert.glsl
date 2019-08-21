// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP3D;
uniform ivec4 uPhysViewport;

attribute vec3 aBase; // address in world space
attribute vec4 aColor;
attribute vec2 aTexCoord;
attribute vec2 aVert; // offset in raw pixels

varying vec4 vColor;
varying vec2 vTexCoord;

// [0, 1]^2 to pixels
vec2 convertScreen01toPhysPixels(vec2 pos)
{
    return pos * vec2(uPhysViewport.zw) + vec2(uPhysViewport.xy);
}

vec2 convertPhysPixelsToScreen01(vec2 pixels)
{
    return (pixels - vec2(uPhysViewport.xy)) / vec2(uPhysViewport.zw);
}

vec2 anti_flicker(vec2 pos)
{
    return convertPhysPixelsToScreen01(floor(convertScreen01toPhysPixels(pos)));
}

// [-1, 1]^2 to [0, 1]^2
vec2 convertNDCtoScreenSpace(vec2 pos)
{
    return pos * 0.5 + 0.5;
}

// [0, 1]^2 to [-1, 1]^2
vec2 convertScreen01toNdcClip(vec2 pos)
{
    pos = pos * 2.0 - 1.0;
    return pos;
}

vec2 addPerVertexOffset(vec2 wordOriginNdc)
{
    vec2 wordOriginScreen = anti_flicker(convertNDCtoScreenSpace(wordOriginNdc));
    return convertScreen01toNdcClip(wordOriginScreen + convertPhysPixelsToScreen01(aVert.xy));
}

// NOTE:
// 1. Returning identical coordinates will yield degenerate
//    triangles, so no fragments will be generated.
// 2. Returning a value that is outside the clip region
//    might help some implementations bail out sooner.
//    (hint: 2 is outside the clip region [-1, 1])
const vec4 ignored = vec4(2.0, 2.0, 2.0, 1.0);

vec4 computePosition()
{
#if 0 // REVISIT: add a maximum view distance?
    if (length(aBase - uCenter) > uMaxViewDistance) {
        return ignored;
    }
#endif

    vec4 pos = uMVP3D * vec4(aBase, 1); /* 3D transform */

    // Ignore any text that falls far outside of the clip region.
    if (any(greaterThan(abs(pos.xyz), vec3(1.5 * abs(pos.w))))) {
        return ignored;
    }

    // Also ignore anything that appears too close to the camera.
    if (abs(pos.w) < 1e-3) {
        return ignored;
    }

    // convert from clip space to normalized device coordinates (NDC)
    pos /= pos.w;

    pos.xy = addPerVertexOffset(pos.xy);

    // Note: We're not using the built-in MVP matrix.
    return pos;
}

void main()
{
    vColor = aColor;
    vTexCoord = aTexCoord;
    gl_Position = computePosition();
}
