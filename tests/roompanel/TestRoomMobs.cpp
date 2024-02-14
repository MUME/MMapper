// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "TestRoomMobs.h"

#include <QtTest>

#include "roompanel/RoomMobs.h"

TestRoomMobs::TestRoomMobs() = default;

TestRoomMobs::~TestRoomMobs() = default;

void TestRoomMobs::testAddMob()
{
    RoomMobs mobs(nullptr);
    RoomMobUpdate mobUpdate;
    mobUpdate.setId(1);
    mobs.addMob(std::move(mobUpdate));
    QVERIFY(mobs.isIdPresent(1));
}

void TestRoomMobs::testRemoveMobById()
{
    RoomMobs mobs(nullptr);
    RoomMobUpdate mobUpdate;
    mobUpdate.setId(1);
    mobs.addMob(std::move(mobUpdate));

    bool removed = mobs.removeMobById(1);

    QVERIFY(removed);
    QVERIFY(!mobs.isIdPresent(1));
}

void TestRoomMobs::testUpdateMob()
{
    RoomMobs mobs(nullptr);

    // Create and add a mob with initial settings
    RoomMobUpdate mobUpdate1;
    mobUpdate1.setId(1);
    mobUpdate1.setField(MobFieldEnum::NAME, QVariant("OriginalName"));

    // Manually create and set flags for mobUpdate1
    MobFieldFlags flags1;
    flags1 |= MobFieldEnum::NAME;
    mobUpdate1.setFlags(flags1);

    mobs.addMob(std::move(mobUpdate1));

    // Create a second update for the same mob but with a changed field
    RoomMobUpdate mobUpdate2;
    mobUpdate2.setId(1);
    mobUpdate2.setField(MobFieldEnum::NAME, QVariant("UpdatedName"));

    // Manually create and set flags for mobUpdate2
    MobFieldFlags flags2;
    flags2 |= MobFieldEnum::NAME;
    mobUpdate2.setFlags(flags2);

    // Attempt to update the mob with the new details
    bool updated = mobs.updateMob(std::move(mobUpdate2));

    // Verify the update was successful
    QVERIFY(updated);

    auto updatedMob = mobs.getMobById(1);
    QVERIFY(updatedMob);
    QVERIFY(updatedMob->getField(MobFieldEnum::NAME).toString() == "UpdatedName");
}

void TestRoomMobs::testResetMobs()
{
    RoomMobs mobs(nullptr);
    RoomMobUpdate mobUpdate;
    mobUpdate.setId(1);
    mobs.addMob(std::move(mobUpdate));

    mobs.resetMobs();

    QVERIFY(!mobs.isIdPresent(1));
}

QTEST_MAIN(TestRoomMobs)
