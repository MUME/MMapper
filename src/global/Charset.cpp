// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Charset.h"

#include "../parser/parserutils.h"
#include "TextUtils.h"

void latin1ToUtf8(std::ostream &os, const char c)
{
    const auto uc = static_cast<uint8_t>(c);
    // U+0000 to U+007F: 0xxxxxxx (7 bits)
    if (uc < 0x80u) {
        os << c;
        return;
    }

    // U+0080 .. U+07FF: 110xxxxx  10xxxxxx (11 bits)
    //
    // but we only care about a smaller subset:
    // U+0080 .. U+00FF: 1100001x  10xxxxxx (7 bits)
    //                    c2..c3    80..bf  (hex)
    //
    // 0x80 becomes         c2        80
    // 0xff becomes         c3        bf
    static constexpr const auto SIX_BIT_MASK = (1u << 6u) - 1u;
    static_assert(SIX_BIT_MASK == 63);
    char buf[3];
    buf[0] = char(0xc0u | (uc >> 6u));          // c2..c3
    buf[1] = char(0x80u | (uc & SIX_BIT_MASK)); // 80..bf
    buf[2] = C_NUL;
    os.write(buf, 2);
}

void latin1ToUtf8(std::ostream &os, const std::string_view &sv)
{
    for (char c : sv) {
        latin1ToUtf8(os, c);
    }
}

void convertFromLatin1(std::ostream &os,
                       const CharacterEncodingEnum encoding,
                       const std::string_view &sv)
{
    switch (encoding) {
    case CharacterEncodingEnum::ASCII:
        ParserUtils::latin1ToAscii(os, sv);
        return;
    case CharacterEncodingEnum::LATIN1:
        os << sv;
        return;
    case CharacterEncodingEnum::UTF8:
        latin1ToUtf8(os, sv);
        return;
    }

    std::abort();
}
