// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RoomIdSet.h"

#include "../global/tests.h"

namespace test {
void testRoomIdSet()
{
    RoomIdSet a;
    RoomIdSet b;

    TEST_ASSERT(a == b);
    TEST_ASSERT(!a.contains(RoomId(1)));

    a.insert(RoomId(1));

    TEST_ASSERT(a.contains(RoomId(1)));
    TEST_ASSERT(a != b);

    b.insert(RoomId(1));
    TEST_ASSERT(!a.containsElementNotIn(b));
    TEST_ASSERT(a == b);

    a.insert(RoomId(7));
    TEST_ASSERT(a.containsElementNotIn(b));
    TEST_ASSERT(a != b);

    b.insert(RoomId(7));
    TEST_ASSERT(!a.containsElementNotIn(b));
    TEST_ASSERT(a == b);

    b.erase(RoomId(7));
    TEST_ASSERT(a.containsElementNotIn(b));
    TEST_ASSERT(a != b);

    b.insert(RoomId(8));
    TEST_ASSERT(a.containsElementNotIn(b));
    TEST_ASSERT(a != b);

    b.insert(RoomId(7));
    TEST_ASSERT(!a.containsElementNotIn(b));
    TEST_ASSERT(a != b);
}
} // namespace test
