// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "TaggedString.h"

#include "tests.h"

namespace test {

namespace { // anonymous
struct NODISCARD FakeTag final
{
    NODISCARD static bool isValid(std::string_view) { return true; }
};
struct NODISCARD FakeTag2 final
{
    NODISCARD static bool isValid(std::string_view) { return true; }
};
} // namespace

void testTaggedString()
{
    // constexpr const char latin1[]{"latin1\xFF"};
    constexpr const char utf8[]{"utf8\xC3\xBF"};
    {
        MAYBE_UNUSED auto ignored = TaggedStringUtf8<FakeTag>{utf8}.getStdStringUtf8();
        TEST_ASSERT(ignored == utf8);
    }
    {
        MAYBE_UNUSED auto ignored = TaggedBoxedStringUtf8<FakeTag>{utf8}.toStdStringUtf8();
        TEST_ASSERT(ignored == utf8);
    }
    {
        MAYBE_UNUSED auto ignored = TaggedBoxedStringUtf8<FakeTag>{
            TaggedStringUtf8<FakeTag2>{
                utf8}}.toStdStringUtf8();
        TEST_ASSERT(ignored == utf8);
    }
}
} // namespace test
