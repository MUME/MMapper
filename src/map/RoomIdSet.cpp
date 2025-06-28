// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RoomIdSet.h"

#include "../global/tests.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace test {

template<typename Type>
void runRoomIdSetTests()
{
    Type defaultConstructorSet;
    TEST_ASSERT(defaultConstructorSet.empty());
    TEST_ASSERT(defaultConstructorSet.size() == 0ULL);
    TEST_ASSERT(!defaultConstructorSet.contains(RoomId(1)));
    TEST_ASSERT(defaultConstructorSet.begin() == defaultConstructorSet.end());

    RoomId singleId(42);
    Type setWithSingleId(singleId);
    TEST_ASSERT(!setWithSingleId.empty());
    TEST_ASSERT(setWithSingleId.size() == 1ULL);
    TEST_ASSERT(setWithSingleId.contains(singleId));
    TEST_ASSERT(!setWithSingleId.contains(RoomId(1)));
    TEST_ASSERT(*setWithSingleId.begin() == singleId);
    TEST_ASSERT(*setWithSingleId.begin() == singleId);

    Type setForInsert;
    setForInsert.insert(RoomId(10));
    TEST_ASSERT(!setForInsert.empty());
    TEST_ASSERT(setForInsert.size() == 1ULL);
    TEST_ASSERT(setForInsert.contains(RoomId(10)));
    setForInsert.insert(RoomId(20));
    TEST_ASSERT(setForInsert.size() == 2ULL);
    TEST_ASSERT(setForInsert.contains(RoomId(20)));
    setForInsert.insert(RoomId(10));
    TEST_ASSERT(setForInsert.size() == 2ULL);

    Type setForErase;
    setForErase.insert(RoomId(10));
    setForErase.insert(RoomId(20));
    setForErase.erase(RoomId(10));
    TEST_ASSERT(setForErase.size() == 1ULL);
    TEST_ASSERT(!setForErase.contains(RoomId(10)));
    setForErase.erase(RoomId(30));
    TEST_ASSERT(setForErase.size() == 1ULL);

    Type setForClear;
    setForClear.insert(RoomId(20));
    setForClear.clear();
    TEST_ASSERT(setForClear.empty());
    TEST_ASSERT(setForClear.size() == 0ULL);
    TEST_ASSERT(!setForClear.contains(RoomId(20)));

    Type setForIteration;
    setForIteration.insert(RoomId(5));
    setForIteration.insert(RoomId(15));
    setForIteration.insert(RoomId(10));
    std::vector<RoomId> sortedElements;
    for (const auto &id : setForIteration) {
        sortedElements.push_back(id);
    }
    std::sort(sortedElements.begin(), sortedElements.end());
    TEST_ASSERT(sortedElements.size() == 3ULL);
    TEST_ASSERT(sortedElements[0] == RoomId(5));
    TEST_ASSERT(sortedElements[1] == RoomId(10));
    TEST_ASSERT(sortedElements[2] == RoomId(15));

    Type setEqualToIteratorTestSet;
    setEqualToIteratorTestSet.insert(RoomId(5));
    setEqualToIteratorTestSet.insert(RoomId(10));
    setEqualToIteratorTestSet.insert(RoomId(15));
    TEST_ASSERT(setForIteration == setEqualToIteratorTestSet);
    TEST_ASSERT(!(setForIteration != setEqualToIteratorTestSet));

    Type setUnequalToIteratorTestSet;
    setUnequalToIteratorTestSet.insert(RoomId(5));
    setUnequalToIteratorTestSet.insert(RoomId(10));
    TEST_ASSERT(setForIteration != setUnequalToIteratorTestSet);
    TEST_ASSERT(!(setForIteration == setUnequalToIteratorTestSet));

    Type emptyTestSet;
    TEST_ASSERT(defaultConstructorSet == emptyTestSet);
    TEST_ASSERT(!(defaultConstructorSet != emptyTestSet));

    TEST_ASSERT(!setForIteration.containsElementNotIn(setEqualToIteratorTestSet));
    TEST_ASSERT(setForIteration.containsElementNotIn(setUnequalToIteratorTestSet));
    TEST_ASSERT(!setUnequalToIteratorTestSet.containsElementNotIn(setForIteration));
    TEST_ASSERT(setForIteration.containsElementNotIn(defaultConstructorSet));
    TEST_ASSERT(!defaultConstructorSet.containsElementNotIn(setForIteration));
    TEST_ASSERT(!defaultConstructorSet.containsElementNotIn(emptyTestSet));
    TEST_ASSERT(!setForIteration.containsElementNotIn(setForIteration));

    Type set1ForInsertAll;
    set1ForInsertAll.insert(RoomId(1));
    set1ForInsertAll.insert(RoomId(2));
    Type set2ForInsertAll;
    set2ForInsertAll.insert(RoomId(2));
    set2ForInsertAll.insert(RoomId(3));
    set1ForInsertAll.insertAll(set2ForInsertAll);
    TEST_ASSERT(set1ForInsertAll.size() == 3ULL);
    TEST_ASSERT(set1ForInsertAll.contains(RoomId(1)));
    TEST_ASSERT(set1ForInsertAll.contains(RoomId(2)));
    TEST_ASSERT(set1ForInsertAll.contains(RoomId(3)));

    Type nonEmptySetForInsertAll;
    nonEmptySetForInsertAll.insert(RoomId(100));
    Type emptySetForInsertAll;
    nonEmptySetForInsertAll.insertAll(emptySetForInsertAll);
    TEST_ASSERT(nonEmptySetForInsertAll.size() == 1ULL);
    TEST_ASSERT(nonEmptySetForInsertAll.contains(RoomId(100)));

    Type emptySetAForInsertAll;
    Type emptySetBForInsertAll;
    emptySetAForInsertAll.insertAll(emptySetBForInsertAll);
    TEST_ASSERT(emptySetAForInsertAll.empty());

    Type setForFirstLast;
    setForFirstLast.insert(RoomId(50));
    setForFirstLast.insert(RoomId(30));
    setForFirstLast.insert(RoomId(70));
    TEST_ASSERT(setForFirstLast.first() == RoomId(30));
    TEST_ASSERT(setForFirstLast.last() == RoomId(70));

    Type emptySetForExceptionTest;
    bool exceptionCaughtFirst = false;
    try {
        std::ignore = emptySetForExceptionTest.first();
    } catch (const std::out_of_range &) {
        exceptionCaughtFirst = true;
    }
    TEST_ASSERT(exceptionCaughtFirst);

    bool exceptionCaughtLast = false;
    try {
        std::ignore = emptySetForExceptionTest.last();
    } catch (const std::out_of_range &) {
        exceptionCaughtLast = true;
    }
    TEST_ASSERT(exceptionCaughtLast);
}

void testRoomIdSet()
{
    runRoomIdSetTests<RoomIdSet>();
};

void testImmRoomIdSet()
{
    runRoomIdSetTests<ImmRoomIdSet>();
};
} // namespace test
