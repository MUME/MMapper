// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

layout(std140) uniform CameraBlock
{
    mat4 uViewProj;
    vec4 uPlayerPos;        // xyz, w=zScale
};

out vec3 vWorldPos;

void main()
{
    // Quad vertices 0..3 for TRIANGLE_STRIP
    // 0: (-1, -1), 1: (1, -1), 2: (-1, 1), 3: (1, 1)
    vec2 offset = vec2(float(gl_VertexID & 1) * 2.0 - 1.0,
                       float((gl_VertexID >> 1) & 1) * 2.0 - 1.0);

    // Large enough to cover the mask radius (WEATHER_RADIUS is currently 14.0)
    vec2 worldXY = uPlayerPos.xy + offset * WEATHER_RADIUS;
    vWorldPos = vec3(worldXY, uPlayerPos.z);

    // Project to screen space
    gl_Position = uViewProj * vec4(vWorldPos.x, vWorldPos.y, vWorldPos.z * uPlayerPos.w, 1.0);
}
