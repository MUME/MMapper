// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform vec4 uColor;

varying vec4 vColor;

void main()
{
    gl_FragColor = vColor * uColor;
}
