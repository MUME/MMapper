// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

uniform mat4 uMVP;
uniform NamedColorsBlock {
    vec4 uNamedColors[MAX_NAMED_COLORS];
};

layout(location = 0) in ivec4 aVertTexCol;

out vec4 vColor;
out vec3 vTexCoord;

void main()
{
    // ccw-order assumes it's a triangle fan (as opposed to a triangle strip)
    const ivec3[4] ioffsets_ccw = ivec3[4](ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(1, 1, 0), ivec3(0, 1, 0));
    ivec3 ioffset = ioffsets_ccw[gl_VertexID];

    int texZ = aVertTexCol.w & 0xFF;
    int colorId = (aVertTexCol.w >> 8) % MAX_NAMED_COLORS;

    vColor = uNamedColors[colorId];
    vTexCoord = vec3(ioffset.xy, float(texZ));
    gl_Position = uMVP * vec4(aVertTexCol.xyz + ioffset, 1.0);
}
