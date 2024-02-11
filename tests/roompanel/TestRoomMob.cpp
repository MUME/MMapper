// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "TestRoomMob.h"

#include <QtTest>

#include "roompanel/RoomMob.h"

TestRoomMob::TestRoomMob() = default;

TestRoomMob::~TestRoomMob() = default;

void TestRoomMob::testInitialization()
{
    RoomMobData mobData;
    QCOMPARE(mobData.getId(), RoomMobData::NOID);

    // Verify that all fields are initialized to empty QVariant
    for (uint8_t i = 0; i < NUM_MOB_FIELDS; ++i) {
        QVERIFY(mobData.getField(static_cast<MobFieldEnum>(i)).isNull());
    }
}

void TestRoomMob::testSetGetId()
{
    RoomMobData mobData;
    RoomMobData::Id testId = 123;
    mobData.setId(testId);
    QCOMPARE(mobData.getId(), testId);
}

void TestRoomMob::testSetGetField()
{
    // Create an instance of RoomMobData to test
    RoomMobData mobData;

    // Prepare a test value to set for the mob's name field
    QVariant testValue = QVariant::fromValue(QString("MobName"));

    // Set the NAME field of mobData to the test value
    mobData.setField(MobFieldEnum::NAME, testValue);

    // Assert that the value retrieved from the NAME field is equal to the test value.
    // This checks if the setField and getField methods work correctly.
    QCOMPARE(mobData.getField(MobFieldEnum::NAME), testValue);
}

void TestRoomMob::testAllocAndUpdate()
{
    auto roomMob = RoomMob::alloc();
    QVERIFY(roomMob != nullptr);

    RoomMobUpdate update;
    update.setId(roomMob->getId());
    QVariant newValue = QVariant::fromValue(QString("SomeMobName"));
    update.setField(MobFieldEnum::NAME, newValue);
    update.setFlags(RoomMobUpdate::Flags(MobFieldEnum::NAME));

    bool updated = roomMob->updateFrom(std::move(update));
    QVERIFY(updated);
    QCOMPARE(roomMob->getField(MobFieldEnum::NAME), newValue);
}

void TestRoomMob::testFlagsAndFields()
{
    RoomMobUpdate update;

    update.setFlags(MobFieldFlags(MobFieldEnum::NAME)); // Directly set using constructor

    QVERIFY(update.getFlags().contains(MobFieldEnum::NAME));
    QVERIFY(!update.getFlags().contains(MobFieldEnum::DESC));

    QVariant testNameValue = QVariant::fromValue(QString("TestName"));
    update.setField(MobFieldEnum::NAME, testNameValue);

    // Verify the field is correctly updated
    QCOMPARE(update.getField(MobFieldEnum::NAME), testNameValue);
}

QTEST_MAIN(TestRoomMob)
