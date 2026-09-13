// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "TestRoomManager.h"

#include "../src/global/Charset.h"
#include "../src/global/tests.h"
#include "../src/proxy/GmcpMessage.h"
#include "../src/roompanel/RoomManager.h"
#include "../src/roompanel/RoomModel.h"

#include <QColor>
#include <QtTest>
namespace { // anonymous
const auto mysterieuse = std::invoke([]() -> QString {
    const QString result = "myst\u00E9rieuse cr\u00E9ature";
    {
        const auto expected_ascii = "mysterieuse creature";
        auto copy = result;
        mmqt::toAsciiInPlace(copy);
        TEST_ASSERT(copy == expected_ascii);
    }
    {
        TEST_ASSERT(result.size() == 20);
        TEST_ASSERT(result[4].unicode() == 0xE9u);
    }
    {
        const auto latin1 = mmqt::toStdStringLatin1(result); // sanity checking
        TEST_ASSERT(latin1.size() == 20);
        TEST_ASSERT(latin1[4] == '\xE9');
    }
    {
        const auto utf8 = mmqt::toStdStringUtf8(result);
        TEST_ASSERT(utf8.size() == 22);
        TEST_ASSERT(utf8[4] == '\xC3');
        TEST_ASSERT(utf8[5] == '\xA9');
    }
    return result;
});

const QJsonObject gmcpRoomCharsAddObj = {{"desc",
                                          "A magpie is flying around looking for some food."},
                                         {"flags", QJsonArray()},
                                         {"id", 2},
                                         {"labels", QJsonArray()},
                                         {"name", mysterieuse},
                                         {"position", "standing"},
                                         {"type", "npc"}};

NODISCARD GmcpJson makeCompactJson(const QJsonObject &x)
{
    return GmcpJson{QJsonDocument(x).toJson(QJsonDocument::Compact).toStdString()};
}
} // namespace

TestRoomManager::TestRoomManager(RoomManager &manager)
    : m_manager{manager}
{}

TestRoomManager::~TestRoomManager() = default;

void TestRoomManager::init() {}

void TestRoomManager::cleanup() {}

void TestRoomManager::testSlotReset()
{
    auto &manager = m_manager;
    QVERIFY(!manager.getRoom().isIdPresent(2));

    auto jsonStr = makeCompactJson(gmcpRoomCharsAddObj);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);

    manager.slot_parseGmcpInput(addMessage);
    QVERIFY(manager.getRoom().isIdPresent(2)); // Check if mob with ID 2 is present

    manager.slot_reset();
    QVERIFY(!manager.getRoom().isIdPresent(2)); // Verify ID 2 is not present after reset
}

void TestRoomManager::testParseGmcpAddValidMessage()
{
    auto jsonStr = makeCompactJson(gmcpRoomCharsAddObj);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);

    auto &manager = m_manager;
    QSignalSpy updateWidgetSpy(&manager, &RoomManager::sig_updateWidget);
    manager.slot_parseGmcpInput(addMessage);
    QCOMPARE(updateWidgetSpy.count(), 1);
    QVERIFY(manager.getRoom().isIdPresent(2)); // Verify mob was added correctly
}

void TestRoomManager::testParseGmcpInvalidMessage()
{
    // Create an invalid GMCP message (e.g., missing required fields)
    QJsonObject invalidObj = {{"invalidField", "value"}};
    auto jsonStr = makeCompactJson(invalidObj);
    GmcpMessage invalidMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);

    // Attempt to parse the invalid message
    auto &manager = m_manager;
    QSignalSpy updateWidgetSpy(&manager, &RoomManager::sig_updateWidget);
    manager.slot_parseGmcpInput(invalidMessage);

    // Verify that the widget update signal was not emitted and no mobs were added
    QCOMPARE(updateWidgetSpy.count(), 0);
}

void TestRoomManager::testParseGmcpUpdateValidMessage()
{
    // Step 1: Add a mob to ensure there's something to update
    QJsonObject addObj = {{"id", 2}, {"name", "male magpie"}, {"position", "standing"}};
    auto addJsonStr = makeCompactJson(addObj);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, addJsonStr);
    auto &manager = m_manager;
    manager.slot_parseGmcpInput(addMessage);
    QVERIFY(manager.getRoom().isIdPresent(2)); // Verify mob was added correctly

    // Step 2: Create an update message for the same mob with new information
    QJsonObject updateObj = {{"id", 2}, {"name", "angry male magpie"}, {"position", "sleeping"}};
    auto updateJsonStr = makeCompactJson(updateObj);
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

void TestRoomManager::testRoomModelEnemyColors()
{
    auto &manager = m_manager;
    manager.slot_reset();

    RoomModel model(nullptr, manager.getRoom());
    model.update();

    // Row 0 is the placeholder empty row when there are no mobs.
    const QModelIndex normalName = model.index(0, static_cast<int>(RoomModel::ColumnTypeEnum::NAME));
    QVERIFY(!model.data(normalName, RoomModel::RowBackgroundRole).isValid());
    QVERIFY(!model.data(normalName, RoomModel::RowForegroundRole).isValid());

    // A name wrapped in asterisks (e.g. "*an Orc*") marks an enemy player.
    QJsonObject enemyObj = {{"id", 3}, {"name", "*an Orc*"}, {"position", "standing"}};
    auto jsonStr = makeCompactJson(enemyObj);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);
    manager.slot_parseGmcpInput(addMessage);
    model.update();

    const QModelIndex enemyName = model.index(0, static_cast<int>(RoomModel::ColumnTypeEnum::NAME));
    const QVariant background = model.data(enemyName, RoomModel::RowBackgroundRole);
    QVERIFY(background.isValid());
    QCOMPARE(background.value<QColor>(), QColor(Qt::yellow));

    const QVariant foreground = model.data(enemyName, RoomModel::RowForegroundRole);
    QVERIFY(foreground.isValid());

    // The LABEL column is explicitly excluded from enemy highlighting.
    const QModelIndex enemyLabel = model.index(0,
                                               static_cast<int>(RoomModel::ColumnTypeEnum::LABEL));
    QVERIFY(!model.data(enemyLabel, RoomModel::RowBackgroundRole).isValid());
}

void TestRoomManager::testRoomModelFightingYouColors()
{
    auto &manager = m_manager;
    manager.slot_reset();

    QJsonObject fightingObj = {{"id", 4},
                               {"name", "an angry troll"},
                               {"position", "standing"},
                               {"fighting", "you"}};
    auto jsonStr = makeCompactJson(fightingObj);
    GmcpMessage addMessage(GmcpMessageTypeEnum::ROOM_CHARS_ADD, jsonStr);
    manager.slot_parseGmcpInput(addMessage);

    RoomModel model(nullptr, manager.getRoom());
    model.update();

    const QModelIndex fightingIndex = model.index(0,
                                                  static_cast<int>(
                                                      RoomModel::ColumnTypeEnum::FIGHTING));
    QCOMPARE(model.data(fightingIndex, Qt::DisplayRole).toString(), QStringLiteral("YOU"));

    const QVariant foreground = model.data(fightingIndex, RoomModel::RowForegroundRole);
    QVERIFY(foreground.isValid());
    QCOMPARE(foreground.value<QColor>(), QColor(Qt::red));
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    RoomManager manager(nullptr);
    TestRoomManager test(manager);

    return QTest::qExec(&test, argc, argv);
}
