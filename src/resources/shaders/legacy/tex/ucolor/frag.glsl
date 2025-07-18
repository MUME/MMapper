// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

uniform sampler2DArray uTexture;
uniform vec4 uColor;

in vec3 vTexCoord;

out vec4 vFragmentColor;

void main()
{
    vFragmentColor = uColor * texture(uTexture, vTexCoord);
}
