// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP;

attribute vec3 aVert;

void main()
{
    gl_Position = uMVP * vec4(aVert, 1.0);
}
