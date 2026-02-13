// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

uniform sampler2D uTexture;

in vec2 vTexCoord;

out vec4 vFragmentColor;

void main()
{
    vFragmentColor = texture(uTexture, vTexCoord);
}
