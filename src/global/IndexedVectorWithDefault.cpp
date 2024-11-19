// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "IndexedVectorWithDefault.h"

#include "TaggedInt.h"
#include "tests.h"

namespace test {

namespace { // anonymous
namespace tags {
struct NODISCARD MyTag final
{};
} // namespace tags

struct NODISCARD MyTaggedInt final : TaggedInt<MyTaggedInt, tags::MyTag, uint32_t>
{
    using TaggedInt::TaggedInt;
};

void test_grow_to_include()
{
    static constexpr int DEFVAL = 42;
    IndexedVectorWithDefault<int, MyTaggedInt> vec{DEFVAL};
    vec.grow_to_include(3);
    assert(vec.size() == 4);
    for (uint32_t i = 0; i < 4; ++i) {
        TEST_ASSERT(vec.at(MyTaggedInt{i}) == DEFVAL);
    }
}
void test_grow_to_size()
{
    static constexpr int DEFVAL = 42;
    IndexedVectorWithDefault<int, MyTaggedInt> vec{DEFVAL};
    vec.grow_to_size(3);
    assert(vec.size() == 3);
    for (uint32_t i = 0; i < 3; ++i) {
        TEST_ASSERT(vec.at(MyTaggedInt{i}) == DEFVAL);
    }
}
} // namespace

void testIndexedVectorWithDefault()
{
    test_grow_to_include();
    test_grow_to_size();
}

} // namespace test
