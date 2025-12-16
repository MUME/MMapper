// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform vec3 uPlayerPos;      // Player position in world space (x, y, z)
uniform int uCurrentLayer;    // Current focused layer (player's Z)
uniform int uEnableRadial;    // 1 to enable radial transparency, 0 to disable
uniform int uTexturesDisabled; // 1 if textures are disabled (non-textured mode), 0 otherwise
uniform int uIsNight;         // 1 if night time, 0 otherwise

varying vec4 vColor;
varying vec2 vTexCoord;
varying vec3 vWorldPos;

// 2D noise function (based on value noise)
float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    // Smooth interpolation (smoothstep)
    f = f * f * (3.0 - 2.0 * f);

    // Hash function for corners
    float a = fract(sin(dot(i, vec2(127.1, 311.7))) * 43758.5453);
    float b = fract(sin(dot(i + vec2(1.0, 0.0), vec2(127.1, 311.7))) * 43758.5453);
    float c = fract(sin(dot(i + vec2(0.0, 1.0), vec2(127.1, 311.7))) * 43758.5453);
    float d = fract(sin(dot(i + vec2(1.0, 1.0), vec2(127.1, 311.7))) * 43758.5453);

    // Bilinear interpolation
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal noise (multiple octaves for more detail)
float fractalNoise(vec2 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 4; i++) {
        if (i >= octaves) break;
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// Organic pattern for transparent layers (like FastNoise)
float organicPattern(vec2 pos, float scale, float threshold)
{
    // Use fractal noise (2 octaves) for organic, irregular patterns with moderate detail
    // This creates smaller transparency patches with sharper edges
    float n = fractalNoise(pos * scale, 2);  // 2 octaves for balanced organic variation

    // Create holes/gaps based on threshold with sharper transition for defined edges
    // Narrower smoothstep creates sharper, more defined transitions
    return smoothstep(threshold - 0.12, threshold + 0.12, n);
}

void main()
{
    vec4 texColor = vColor * uColor * texture2D(uTexture, vTexCoord);

    // Only apply radial transparency if enabled and NOT on the focused layer
    if (uEnableRadial == 1) {
        int layerOffset = int(vWorldPos.z) - uCurrentLayer;
        int layerDistance = int(abs(float(layerOffset)));

        // Determine if this is an upper or lower layer
        bool isUpperLayer = layerOffset > 0;
        bool isLowerLayer = layerOffset < 0;

        // Skip masking entirely for elements on the focused layer
        if (layerDistance > 0) {
            // Calculate grid-aligned distance (Chebyshev distance for room-based grid)
            float distX = abs(vWorldPos.x - uPlayerPos.x);
            float distY = abs(vWorldPos.y - uPlayerPos.y);
            float distBase = max(distX, distY);  // Max gives us a square/cubic region aligned to the room grid

            // For lower layers: use uniform opacity based only on layer distance
            if (isLowerLayer) {
                // Simple layer-based transparency: fade based on how many layers below
                float baseAlpha;

                if (layerDistance <= 2) {
                    // 1-2 layers below: more visible
                    baseAlpha = 0.65;
                } else if (layerDistance <= 5) {
                    // 3-5 layers below: moderate visibility
                    baseAlpha = 0.45;
                } else if (layerDistance <= 10) {
                    // 6-10 layers below: low visibility
                    baseAlpha = 0.25;
                } else {
                    // More than 10 layers below: invisible
                    baseAlpha = 0.0;
                }

                // Apply uniform alpha across the entire layer
                texColor.a = min(texColor.a, baseAlpha);
            }
            // For upper layers: apply organic distortion and organic pattern mask
            else if (isUpperLayer) {
                // Add organic distortion to the distance to make edges irregular and natural
                // Use three noise frequencies for highly irregular, jagged edge variation with big waves
                float edgeNoise1 = fractalNoise(vWorldPos.xy * 0.4, 2);   // Large-scale variation (doubled frequency)
                float edgeNoise2 = fractalNoise(vWorldPos.xy * 1.2, 2);   // Medium-scale variation (doubled frequency)
                float edgeNoise3 = fractalNoise(vWorldPos.xy * 3.0, 3);   // High-frequency for very jagged edges (doubled frequency)
                // Combine noise layers with doubled amplitude for sharper, bigger waves
                float distortion = ((edgeNoise1 - 0.5) * 1.0) + ((edgeNoise2 - 0.5) * 0.7) + ((edgeNoise3 - 0.5) * 0.5);  // ±1.1 total range (doubled)
                float dist = distBase + distortion;  // Apply distortion to create highly organic edges with big waves

                // Graduated radial zone with layer visibility based on distance (reduced radius):
                // > 1.3 rooms away: all 10 layers visible (no modification)
                // 0.7-1.3 rooms away: 5 layers visible
                // 0.4-0.7 rooms away: 2 layers visible
                // Close to player (dist < 0.4): only 1 layer visible (includes walls)

                int maxVisibleLayers = 10; // Default: all layers visible

                if (dist < 0.4) {
                    // Close to player (including walls): only 1 layer
                    maxVisibleLayers = 1;
                } else if (dist < 0.7) {
                    // 0.4-0.7 rooms away: 2 layers visible
                    maxVisibleLayers = 2;
                } else if (dist < 1.3) {
                    // 0.7-1.3 rooms away: 5 layers visible
                    maxVisibleLayers = 5;
                }
                // else: > 1.3 rooms away, all 10 layers visible

                if (layerDistance > maxVisibleLayers) {
                    // Hide layers beyond the visible limit for this distance
                    texColor.a = 0.0;
                } else if (maxVisibleLayers < 10) {
                    // Determine base alpha based on layer distance with new gradual fade
                    float baseAlpha;

                    if (layerDistance == 1) {
                        baseAlpha = 0.5;
                    } else if (layerDistance == 2) {
                        baseAlpha = 0.4;
                    } else if (layerDistance == 3) {
                        baseAlpha = 0.3;
                    } else if (layerDistance == 4) {
                        baseAlpha = 0.2;
                    } else if (layerDistance == 5) {
                        baseAlpha = 0.1;
                    } else if (layerDistance == 6) {
                        baseAlpha = 0.08;
                    } else if (layerDistance == 7) {
                        baseAlpha = 0.06;
                    } else if (layerDistance == 8) {
                        baseAlpha = 0.04;
                    } else if (layerDistance == 9) {
                        baseAlpha = 0.02;
                    } else {
                        // 10+ layers above: invisible
                        baseAlpha = 0.0;
                    }

                    // Apply organic noise pattern to visible transparent layers near player
                    // Higher scale for smaller, more defined organic patches
                    float scale = 99.9;  // Smaller organic patches with sharper edges
                    float threshold = 1.1;  // Adjusted for better balance of visible/transparent areas
                    float pattern = organicPattern(vWorldPos.xy, scale, threshold);
                    // Convert gradient to binary mask for clean transparency
                    // Below 0.5: fully transparent (alpha = 0), Above 0.5: use baseAlpha
                    float holeAlpha = pattern < 0.5 ? 0.0 : 1.0;

                    // Apply pattern to base alpha
                    // Where pattern is low (holes), alpha goes to 0 (fully transparent)
                    // Where pattern is high, use base alpha
                    texColor.a = min(texColor.a, baseAlpha * holeAlpha);
                }
                // else: Far from player (maxVisibleLayers == 10), keep original alpha
            }
        }
    }

    // Apply night darkening to the current layer (20-30% darker)
    if (uIsNight == 1 && int(vWorldPos.z) == uCurrentLayer) {
        texColor.rgb *= 0.75;  // 25% darker at night
    }

    gl_FragColor = texColor;
}
