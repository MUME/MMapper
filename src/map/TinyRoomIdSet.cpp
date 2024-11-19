// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "TinyRoomIdSet.h"

#include "../global/tests.h"
#include "RoomIdSet.h"

#include <array>
#include <set>

template<typename T,
         typename U,
         typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::decay_t<U>>>>
static T convertTo(const U &input)
{
    T copy;
    for (const auto &x : input) {
        copy.insert(x);
    }
    return copy;
}

RoomIdSet toRoomIdSet(const TinyRoomIdSet &set)
{
    return convertTo<RoomIdSet>(set);
}

TinyRoomIdSet toTinyRoomIdSet(const RoomIdSet &set)
{
    return convertTo<TinyRoomIdSet>(set);
}

template<typename Set>
struct NODISCARD TestHelper final
{
public:
    TestHelper() = delete;

public:
    static void test()
    {
        test0();
        test1();
    }

private:
    static void test0()
    {
        constexpr RoomId VAL{42};

        Set set;
        set.insert(VAL);
        TEST_ASSERT(set.size() == 1);
        TEST_ASSERT(set.first() == VAL);
        set.erase(VAL);
        TEST_ASSERT(set.size() == 0);
        TEST_ASSERT(set.empty());
    }

    static void test1()
    {
        constexpr size_t SIZE = 5;
        constexpr size_t KEEP = 1;
        static_assert(SIZE > KEEP);

        Set set;
        for (uint32_t i = 0; i < SIZE; ++i) {
            set.insert(RoomId{i});
            TEST_ASSERT(set.size() == i + 1);
        }

        TEST_ASSERT(set.size() == SIZE);

        {
            std::array<bool, SIZE> found{};
            for (const RoomId x : set) {
                found.at(x.asUint32()) = true;
            }

            for (const bool x : found) {
                TEST_ASSERT(x);
            }
        }

        for (uint32_t i = 0; i < SIZE - KEEP; ++i) {
            set.erase(RoomId{i});
        }

        TEST_ASSERT(set.size() == KEEP);
        {
            Set tmp;
            for (const RoomId x : set) {
                tmp.insert(x);
            }
            TEST_ASSERT(tmp == set);
        }
    }
};

namespace test {
void testTinyRoomIdSet()
{
    static_assert(sizeof(RoomIdSet) > sizeof(TinyRoomIdSet));
    static_assert(sizeof(RoomIdSet) == sizeof(std::set<RoomId>));
    static_assert(sizeof(TinyRoomIdSet) == sizeof(uintptr_t));

    TestHelper<RoomIdSet>::test();
    TestHelper<TinyRoomIdSet>::test();
}
} // namespace test
