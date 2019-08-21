// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP;

attribute vec4 aColor;
attribute vec2 aTexCoord;
attribute vec3 aVert;

varying vec4 vColor;
varying vec2 vTexCoord;

void main()
{
    vColor = aColor;
    vTexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aVert, 1.0);
}
