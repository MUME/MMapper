// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform sampler2D uFontTexture;

varying vec4 vColor;
varying vec2 vTexCoord;

void main()
{
    gl_FragColor = vColor * texture2D(uFontTexture, vTexCoord);
}
