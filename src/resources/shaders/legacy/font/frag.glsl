// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

// The font atlas is a single straight-alpha RGBA texture holding two kinds
// of glyphs:
// - monochrome glyphs: RGB is opaque white and A is the antialiased
//   coverage, so the texel is simply modulated by the text color.
// - color glyphs (e.g. emoji): RGBA holds the actual glyph color, sampled
//   as-is and only faded by the text alpha.
uniform sampler2D uFontTexture;

in vec4 vColor;
in vec2 vTexCoord;
flat in float vIsColor;

out vec4 vFragmentColor;

void main()
{
    vec4 texel = texture(uFontTexture, vTexCoord);
    if (vIsColor > 0.5) {
        vFragmentColor = vec4(texel.rgb, texel.a * vColor.a);
    } else {
        vFragmentColor = vColor * texel;
    }
}
