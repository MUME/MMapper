// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

uniform mat4 uMVP;
layout(std140) uniform NamedColorsBlock
{
    vec4 uNamedColors[MAX_NAMED_COLORS];
};

layout(location = 0) in ivec4 aVertTexCol;

out vec4 vColor;
out vec3 vTexCoord;

void main()
{
#if 0
    // GL_TRIANGLE_FAN
    //  3--2
    //  | /|  Triangles 012 and 023 both use CCW order.
    //  |/ |  Notice that the four vertices are in CCW order.
    //  0--1
    const ivec3[4] ioffsets = ivec3[4](ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(1, 1, 0), ivec3(0, 1, 0)); // fan
#else
    // GL_TRIANGLE_STRIP
    //  2--3  Note: Triangle strips alternate CCW/CW winding on every other triangle. This means...
    //  |\ |    triangle 012 is CCW order, but triangle 123 is CW order (backwards).
    //  | \|  Keep in mind that OpenGL does not actually draw the triangle backwards,
    //  0--1    so it does not affect glFrontFace() or the gl_FrontFacing variable.
    const ivec3[4] ioffsets = ivec3[4](ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(0, 1, 0), ivec3(1, 1, 0)); // strip
#endif
    ivec3 ioffset = ioffsets[gl_VertexID];

    int texZ = aVertTexCol.w & 0xFF;
    int colorId = (aVertTexCol.w >> 8) % MAX_NAMED_COLORS;

    vColor = uNamedColors[colorId];
    vTexCoord = vec3(ioffset.xy, float(texZ));
    gl_Position = uMVP * vec4(aVertTexCol.xyz + ioffset, 1.0);
}
