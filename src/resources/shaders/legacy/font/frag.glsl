// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform sampler2D uFontTexture;

in vec4 vColor;
in vec2 vTexCoord;

out vec4 vFragmentColor;

void main()
{
    vFragmentColor = vColor * texture(uFontTexture, vTexCoord);
}
