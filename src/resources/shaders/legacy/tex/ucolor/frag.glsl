// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform sampler2D uTexture;
uniform vec4 uColor;

varying vec2 vTexCoord;

void main()
{
    gl_FragColor = uColor * texture2D(uTexture, vTexCoord);
}
