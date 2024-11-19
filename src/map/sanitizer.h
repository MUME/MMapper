#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/TaggedString.h"
#include "../global/macros.h"

namespace sanitizer {

namespace tags {
struct NODISCARD SanitizedStringTag final
{
    // each individual sanitized string would have different rules;
    // the tag just helps us avoid shooting ourselves in the foot.
    NODISCARD static bool isValid(std::string_view) { return true; }
};
} // namespace tags

using SanitizedString = TaggedStringUtf8<tags::SanitizedStringTag>;

NODISCARD bool isSanitizedOneLine(std::string_view desc);
NODISCARD SanitizedString sanitizeOneLine(std::string desc);

NODISCARD bool isSanitizedWordWraped(std::string_view desc, size_t width);
NODISCARD SanitizedString sanitizeWordWrapped(std::string desc, size_t width);

NODISCARD bool isSanitizedMultiline(std::string_view desc);
NODISCARD SanitizedString sanitizeMultiline(std::string desc);

NODISCARD bool isSanitizedUserSupplied(std::string_view desc);
NODISCARD SanitizedString sanitizeUserSupplied(std::string desc);

} // namespace sanitizer

namespace test {
extern void testSanitizer();
} // namespace test
