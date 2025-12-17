// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP;

attribute vec3 aVert;

varying vec3 vWorldPos;

void main()
{
    vWorldPos = aVert;
    gl_Position = uMVP * vec4(aVert, 1.0);
}
