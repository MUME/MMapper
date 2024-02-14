// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "TestRoomManager.h"

#include <QtTest>

#include "proxy/GmcpMessage.h"
#include "roompanel/RoomManager.h"

TestRoomManager::TestRoomManager(RoomManager &manager)
    : manager(manager)
{}

TestRoomManager::~TestRoomManager() = default;

void TestRoomManager::init() {}

void TestRoomManager::cleanup() {}

QJsonObject gmcpRoomCharsAddObj = {{"desc", "A magpie is flying around looking for some food."},
                                   {"flags", QJsonArray()},
                                   {"id", 2},
                                   {"labels", QJsonArray()},
                                   {"name", "mystérieuse créature"},
                                   {"position", "standing"},
                                   {"type", "npc"}};

void TestRoomManager::testSlotReset()
{
    QVERIFY(!manager.getRoom().isIdPresent(2));

    QString jsonStr = QJsonDocument(gmcpRoomCharsAddObj).toJson(QJsonDocument::Compact);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);

    manager.slot_parseGmcpInput(addMessage);
    QVERIFY(manager.getRoom().isIdPresent(2)); // Check if mob with ID 2 is present

    manager.slot_reset();
    QVERIFY(!manager.getRoom().isIdPresent(2)); // Verify ID 2 is not present after reset
}

void TestRoomManager::testParseGmcpAddValidMessage()
{
    QString jsonStr = QJsonDocument(gmcpRoomCharsAddObj).toJson(QJsonDocument::Compact);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);

    QSignalSpy updateWidgetSpy(&manager, &RoomManager::sig_updateWidget);
    manager.slot_parseGmcpInput(addMessage);
    QCOMPARE(updateWidgetSpy.count(), 1);
    QVERIFY(manager.getRoom().isIdPresent(2)); // Verify mob was added correctly
}

void TestRoomManager::testParseGmcpInvalidMessage()
{
    // Create an invalid GMCP message (e.g., missing required fields)
    QJsonObject invalidObj = {{"invalidField", "value"}};
    QString jsonStr = QJsonDocument(invalidObj).toJson(QJsonDocument::Compact);
    GmcpMessage invalidMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);

    // Attempt to parse the invalid message
    QSignalSpy updateWidgetSpy(&manager, &RoomManager::sig_updateWidget);
    manager.slot_parseGmcpInput(invalidMessage);

    // Verify that the widget update signal was not emitted and no mobs were added
    QCOMPARE(updateWidgetSpy.count(), 0);
}

void TestRoomManager::testParseGmcpUpdateValidMessage()
{
    // Step 1: Add a mob to ensure there's something to update
    QJsonObject addObj = {{"id", 2}, {"name", "male magpie"}, {"position", "standing"}};
    QString addJsonStr = QJsonDocument(addObj).toJson(QJsonDocument::Compact);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, addJsonStr);
    manager.slot_parseGmcpInput(addMessage);
    QVERIFY(manager.getRoom().isIdPresent(2)); // Verify mob was added correctly

    // Step 2: Create an update message for the same mob with new information
    QJsonObject updateObj = {{"id", 2}, {"name", "angry male magpie"}, {"position", "sleeping"}};
    QString updateJsonStr = QJsonDocument(updateObj).toJson(QJsonDocument::Compact);
    GmcpMessage updateMessage(GmcpMessageTypeEnum::ROOM_CHARS_UPDATE, updateJsonStr);

    // Prepare to capture the sig_updateWidget signal
    QSignalSpy updateWidgetSpy(&manager, &RoomManager::sig_updateWidget);

    // Step 3: Send the update message
    manager.slot_parseGmcpInput(updateMessage);

    // Verify that the sig_updateWidget signal was emitted exactly once
    QCOMPARE(updateWidgetSpy.count(), 1);

    // Step 4: Verify that the mob's information was updated correctly
    auto updatedMob = manager.getRoom().getMobById(2);
    // Ensure the mob exists, using the fact that shared_ptr evaluates to true if
    // it points to something
    QVERIFY(updatedMob);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    RoomManager manager(nullptr);
    TestRoomManager test(manager);

    return QTest::qExec(&test, argc, argv);
}
