// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform vec4 uColor;

in vec4 vColor;

out vec4 vFragmentColor;

void main()
{
    if (dot(gl_PointCoord-0.5, gl_PointCoord-0.5) > 0.25) {
        discard;
    }
    vFragmentColor = vColor * uColor;
}
