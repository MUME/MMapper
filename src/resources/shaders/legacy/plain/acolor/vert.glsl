// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform mat4 uMVP;

layout(location = 0) in vec4 aColor;
layout(location = 1) in vec3 aVert;

out vec4 vColor;

void main()
{
    vColor = aColor;
    gl_Position = uMVP * vec4(aVert, 1.0);
}
