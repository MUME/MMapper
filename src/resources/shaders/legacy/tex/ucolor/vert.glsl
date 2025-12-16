// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP;

attribute vec2 aTexCoord;
attribute vec3 aVert;

varying vec2 vTexCoord;
varying vec3 vWorldPos;

void main()
{
    vTexCoord = aTexCoord;
    vWorldPos = aVert;
    gl_Position = uMVP * vec4(aVert, 1.0);
}
