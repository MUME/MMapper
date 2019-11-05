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
        " during the winter. It would be difficult to climb up there from here, as the"
        " plateau above extends over this area, making a sheltered hollow underneath it"
        " with a depression in the dirt wall at the back. To the east a deep river flows"
        " quickly, while a shallower section lies to the south.");
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
            ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        room->setExitsList(eList);
        return room;
    };

    const auto get_exit_flags = [](const SharedRoom &room) -> ExitsFlagsType {
        ExitsFlagsType flags;
        const ExitsList &list = room->getExitsList();
        for (auto dir : ALL_EXITS7)
            flags.set(dir, list[dir].getExitFlags());
        flags.setValid();
        return flags;
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
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{}); // Remove door and exit flag to the west
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
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::DOWN,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT}); // Remove climb down
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
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::NORTH,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT}); // Remove road north
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setDirectionalLight(ExitDirEnum::NORTH,
                                           DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::EAST, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::WEST, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setDirectionalLight(ExitDirEnum::DOWN, DirectionalLightEnum::DIRECT_SUN_ROOM);
        connectedFlags.setValid();
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

    // Missing exit south
    {
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{}); // Exit is missing from event
        QVERIFY(room->getExitFlags(ExitDirEnum::SOUTH).isExit());
        QVERIFY(!room->getExitFlags(ExitDirEnum::SOUTH).isDoor());
        QVERIFY(!room->getDoorFlags(ExitDirEnum::SOUTH).isHidden());
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("missing exit south") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Missing door and exit down
    {
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{}); // Exit and door are missing from event
        QVERIFY(room->getExitFlags(ExitDirEnum::DOWN).isExit());
        QVERIFY(room->getExitFlags(ExitDirEnum::DOWN).isDoor());
        QVERIFY(!room->getDoorFlags(ExitDirEnum::DOWN).isHidden());
        SharedParseEvent event = ParseEvent::createEvent(CommandEnum::NORTH,
                                                         name,
                                                         dynDesc,
                                                         staticDesc,
                                                         exitFlags,
                                                         PromptFlagsType::fromRoomTerrainType(
                                                             room->getTerrainType()),
                                                         ConnectedRoomFlagsType{});
        QTest::newRow("missing door and exit down")
            << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Missing climb e
    {
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT}); // Remove climb e
        QVERIFY(room->getExitFlags(ExitDirEnum::EAST).isExit());
        QVERIFY(room->getExitFlags(ExitDirEnum::EAST).isClimb()); // Room has climb e
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
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::EAST,
                      ExitFlags{ExitFlagEnum::ROAD | ExitFlagEnum::EXIT}); // Event has road e
        room->setExitFlags(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT});
        QVERIFY(room->getExitFlags(ExitDirEnum::EAST).isExit());
        QVERIFY(!room->getExitFlags(ExitDirEnum::EAST).isRoad()); // Room has no road e
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
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{}); // Exit and door are missing from event
        room->setDoorFlags(ExitDirEnum::WEST,
                           room->getDoorFlags(ExitDirEnum::WEST) ^ DoorFlagEnum::HIDDEN);
        QVERIFY(room->getExitFlags(ExitDirEnum::WEST).isExit());
        QVERIFY(room->getExitFlags(ExitDirEnum::WEST).isDoor());
        QVERIFY(!room->getDoorFlags(ExitDirEnum::WEST).isHidden());
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
        SharedRoom room = create_perfect_room();
        ExitsFlagsType exitFlags = get_exit_flags(room);
        room->setOutDated();
        room->setExitsList(ExitsList{});
        QVERIFY(!room->getExitFlags(ExitDirEnum::SOUTH).isExit());
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
                                                         get_exit_flags(room),
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
                                                         get_exit_flags(room),
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
    auto result = Room::compare(room.get(), *event, matchingTolerance);

    const auto to_str = [](const auto comparison) {
        switch (comparison) {
        case ComparisonResultEnum::EQUAL:
            return "EQUAL";
        case ComparisonResultEnum::DIFFERENT:
            return "DIFFERENT";
        case ComparisonResultEnum::TOLERANCE:
            return "TOLERENACE";
        default:
            break;
        }
        return "?";
    };

    if (comparison != result) {
        static TestRoomAdmin admin;
        qDebug() << "Expected" << to_str(comparison) << "but got" << to_str(result) << "\n"
                 << room->toQString() << "\n"
                 << Room::createTemporaryRoom(admin, *event)->toQString();
    }

    QCOMPARE(result, comparison);
}

QTEST_MAIN(TestExpandoraCommon)
