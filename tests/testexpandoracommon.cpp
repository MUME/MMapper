// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testexpandoracommon.h"

#include <QtTest/QtTest>

#include "../src/expandoracommon/RoomAdmin.h"
#include "../src/expandoracommon/parseevent.h"
#include "../src/expandoracommon/property.h"
#include "../src/expandoracommon/room.h"

TestExpandoraCommon::TestExpandoraCommon() = default;

TestExpandoraCommon::~TestExpandoraCommon() = default;

void TestExpandoraCommon::skippablePropertyTest()
{
    {
        Property property;
        QVERIFY(property.isSkipped());
    }

    {
        Property property{""};
        QVERIFY(property.isSkipped());
    }
}

void TestExpandoraCommon::stringPropertyTest()
{
    const QByteArray ba("hello world");
    Property property(ba.toStdString());
    QVERIFY(!property.isSkipped());
    QVERIFY(property.getStdString() == ba.toStdString());
}

class TestRoomAdmin : public RoomAdmin
{
public:
    void releaseRoom(RoomRecipient &, RoomId) override;
    void keepRoom(RoomRecipient &, RoomId) override {}
    void scheduleAction(const std::shared_ptr<MapAction> &) override {}
};

void TestRoomAdmin::releaseRoom(RoomRecipient &, RoomId) {}

void TestExpandoraCommon::roomCompareTest_data()
{
    QTest::addColumn<SharedRoom>("room");
    QTest::addColumn<SharedParseEvent>("event");
    QTest::addColumn<ComparisonResultEnum>("comparison");

    static TestRoomAdmin admin;
    const RoomName name = RoomName("Riverside");
    const RoomStaticDesc staticDesc = RoomStaticDesc(
        "The high plateau to the north shelters this place from the cold northern winds"
        "during the winter. It would be difficult to climb up there from here, as the"
        "plateau above extends over this area, making a sheltered hollow underneath it"
        "with a depression in the dirt wall at the back. To the east a deep river flows"
        "quickly, while a shallower section lies to the south.");
    const RoomDynamicDesc dynDesc = RoomDynamicDesc("The corpse of a burly orc is lying here.");
    static constexpr const auto terrain = RoomTerrainEnum::FIELD;
    static constexpr const auto lit = RoomLightEnum::LIT;

    const auto create_perfect_room = [&name, &staticDesc, &dynDesc]() -> SharedRoom {
        auto room = Room::createPermanentRoom(admin);
        room->setName(name);
        room->setStaticDescription(staticDesc);
        room->setDynamicDescription(dynDesc);
        room->setTerrainType(terrain);
        room->setLightType(lit);
        room->setUpToDate();
        ExitsList eList;
        eList[ExitDirEnum::NORTH].setExitFlags(
            ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT | ExitFlagEnum::ROAD});
        eList[ExitDirEnum::SOUTH].setExitFlags(ExitFlags{ExitFlagEnum::EXIT});
        eList[ExitDirEnum::EAST].setExitFlags(ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::EXIT});
        eList[ExitDirEnum::WEST].setExitFlags(ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        eList[ExitDirEnum::WEST].setDoorFlags(DoorFlags{DoorFlagEnum::HIDDEN});
        eList[ExitDirEnum::DOWN].setExitFlags(
            ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::CLIMB | ExitFlagEnum::EXIT});
        room->setExitsList(eList);
        return room;
    };

    // Fully Populated
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = Room::getEvent(room.get());
        QTest::newRow("fully populated") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Not Up To Date
    {
        SharedRoom room = create_perfect_room();
        room->setOutDated();
        SharedParseEvent event = Room::getEvent(room.get());
        QTest::newRow("not up to date") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Blinded / Puke in Darkness
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                                         RoomName(""),
                                                         RoomDynamicDesc(""),
                                                         RoomStaticDesc(""),
                                                         ExitsFlagsType{},
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("blinded") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Puke with Awareness in Darkness
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                                         name,
                                                         dynDesc,
                                                         RoomStaticDesc(""),
                                                         ExitsFlagsType{},
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("awareness") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Whitespace change to static desc
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event
            = ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                      name,
                                      dynDesc,
                                      RoomStaticDesc(staticDesc.toQString().simplified()),
                                      ExitsFlagsType{},
                                      PromptFlagsType::fromRoomTerrainType(room->getTerrainType()),
                                      ConnectedRoomFlagsType{});
        QTest::newRow("whitespace") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Single word change to static desc
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event
            = ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                      name,
                                      dynDesc,
                                      RoomStaticDesc(
                                          staticDesc.toQString().replace("difficult", "easy")),
                                      ExitsFlagsType{},
                                      PromptFlagsType::fromRoomTerrainType(room->getTerrainType()),
                                      ConnectedRoomFlagsType{});
        QTest::newRow("single word") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Different room name
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                                         RoomName("Riverbank"),
                                                         dynDesc,
                                                         staticDesc,
                                                         ExitsFlagsType{},
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("different room name") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Different terrain type
    {
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         ExitsFlagsType{},
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             RoomTerrainEnum::UNDEFINED),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("different terrain") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Doors closed hiding exit w
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::NORTH, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("doors closed") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Doors open hiding climb
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::NORTH,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT | ExitFlagEnum::ROAD});
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("doors opens") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Sunlight hiding road
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::NORTH, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT | ExitFlagEnum::CLIMB});
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setDirectionalLight(ExitDirEnum::NORTH,
                                           DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::EAST, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::WEST, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::DOWN, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setValid();
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         connectedFlags);
        QTest::newRow("sunlight hiding road") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Missing non-hidden exit west
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT | ExitFlagEnum::CLIMB});
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setDirectionalLight(ExitDirEnum::NORTH,
                                           DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::EAST, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::WEST, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::DOWN, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setValid();
        SharedRoom room = create_perfect_room();
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         connectedFlags);
        QTest::newRow("missing exit w") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Missing climb e
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::NORTH,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT | ExitFlagEnum::ROAD});
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        SharedRoom room = create_perfect_room();
        room->modifyExitFlags(ExitDirEnum::EAST,
                              FlagModifyModeEnum::SET,
                              ExitFieldVariant{ExitFlags{ExitFlagEnum::EXIT}});
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("missing climb e") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Missing road e
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::NORTH,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT | ExitFlagEnum::ROAD});
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::ROAD | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        SharedRoom room = create_perfect_room();
        room->modifyExitFlags(ExitDirEnum::EAST,
                              FlagModifyModeEnum::SET,
                              ExitFieldVariant{ExitFlags{ExitFlagEnum::EXIT}});
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("missing road n") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Door west not marked as hidden
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::NORTH, ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::EXIT});
        exitFlags.setValid();
        SharedRoom room = create_perfect_room();
        room->modifyExitFlags(ExitDirEnum::WEST,
                              FlagModifyModeEnum::SET,
                              ExitFieldVariant{DoorFlags{}});
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("door west not hidden") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Outdated and no exits (likely player generated room)
    {
        ExitsFlagsType exitFlags;
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{ExitFlagEnum::EXIT});
        exitFlags.setValid();
        SharedRoom room = create_perfect_room();
        room->setOutDated();
        room->setExitsList(ExitsList{});
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("outdated and no exits") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Lit
    {
        SharedRoom room = create_perfect_room();
        PromptFlagsType promptFlags = PromptFlagsType::fromRoomTerrainType(room->getTerrainType());
        promptFlags.setLit();
        room->setLightType(RoomLightEnum::UNDEFINED);
        room->setSundeathType(RoomSundeathEnum::NO_SUNDEATH);
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         ExitsFlagsType{},
                                                         promptFlags,
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("lit") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Dark
    {
        SharedRoom room = create_perfect_room();
        PromptFlagsType promptFlags = PromptFlagsType::fromRoomTerrainType(room->getTerrainType());
        promptFlags.setDark();
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setDirectionalLight(ExitDirEnum::NORTH,
                                           DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setValid();
        room->setLightType(RoomLightEnum::UNDEFINED);
        room->setSundeathType(RoomSundeathEnum::NO_SUNDEATH);
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         ExitsFlagsType{},
                                                         promptFlags,
                                                         connectedFlags);
        QTest::newRow("dark") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // REVISIT: More negative cases
}

void TestExpandoraCommon::roomCompareTest()
{
    QFETCH(SharedRoom, room);
    QFETCH(SharedParseEvent, event);
    QFETCH(ComparisonResultEnum, comparison);

    // REVISIT: Config has default matching tolerance of 8
    const int matchingTolerance = 8;
    QCOMPARE(Room::compare(room.get(), *event, matchingTolerance), comparison);
}

QTEST_MAIN(TestExpandoraCommon)
