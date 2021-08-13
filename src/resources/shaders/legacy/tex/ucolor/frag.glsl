// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform sampler2D uTexture;
uniform vec4 uColor;

in vec2 vTexCoord;

out vec4 vFragmentColor;

void main()
{
    vFragmentColor = uColor * texture(uTexture, vTexCoord);
}
