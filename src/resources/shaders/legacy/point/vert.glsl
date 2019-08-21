// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP;
uniform float uPointSize;

attribute vec4 aColor;
attribute vec3 aVert;

varying vec4 vColor;

void main()
{
    vColor = aColor;
    gl_PointSize = uPointSize;
    gl_Position = uMVP * vec4(aVert, 1.0);
}
