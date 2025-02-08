#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "macros.h"

class QString;

namespace mmqt {
NODISCARD bool containsNonLatin1Codepoints(const QString &s);
// e.g. replace "\u1F44D" with ":+1:"
NODISCARD QString decodeEmojiShortCodes(const QString &s);
// e.g. replace ":+1:" with "\u1F44D"
NODISCARD QString encodeEmojiShortCodes(const QString &s);
} // namespace mmqt

extern void tryLoadEmojis(const QString &filename);

namespace test {
extern void test_emojis();
}
