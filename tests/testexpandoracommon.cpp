// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "testexpandoracommon.h"

#include "../src/global/HideQDebug.h"
#include "../src/global/progresscounter.h"
#include "../src/map/Change.h"
#include "../src/map/Compare.h"
#include "../src/map/mmapper2room.h"

#include <tuple>

#include <QtTest/QtTest>

// TODO: move this?
NODISCARD static QByteArray to_str(const ComparisonResultEnum comparison)
{
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
}

static constexpr const RoomId DEFAULT_ROOMID{0};
static constexpr const ExternalRoomId DEFAULT_EXTERNAL_ROOMID{0};

NODISCARD static RawRoom createTemporaryRoom(const ParseEvent &event)
{
    mmqt::HideQDebug forThisFunction;

    ExternalRawRoom tmp;
    tmp.id = DEFAULT_EXTERNAL_ROOMID;
    tmp.setName(event.getRoomName());
    tmp.setDescription(event.getRoomDesc());
    tmp.setContents(event.getRoomContents());
    auto flags = event.getExitsFlags();
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        tmp.setExitFlags(dir, flags.get(dir));
    }

    ProgressCounter pc;
    MapPair mapPair = Map::fromRooms(pc, {tmp});
    auto &map = mapPair.modified;
    const RoomHandle room1 = map.getRoomHandle(DEFAULT_EXTERNAL_ROOMID);
    std::ignore = map.applySingleChange(pc,
                                        Change{room_change_types::Update{DEFAULT_ROOMID, event}});
    const RoomHandle room2 = map.getRoomHandle(DEFAULT_EXTERNAL_ROOMID);

    assert(room2.getId() == DEFAULT_ROOMID);
    assert(room2.getIdExternal() == DEFAULT_EXTERNAL_ROOMID);

    return room2.getRaw();
}

template<typename... Args>
NODISCARD static ParseEvent createParseEvent(Args &&...args)
{
    // make sure the default command is CommandEnum::UNKNOWN
    if constexpr (utils::are_distinct_v<CommandEnum, utils::remove_cvref_t<Args>...>) {
        return ParseEvent::createEvent2(CommandEnum::UNKNOWN, std::forward<Args>(args)...);
    } else {
        return ParseEvent::createEvent2(std::forward<Args>(args)...);
    }
}

TestExpandoraCommon::TestExpandoraCommon() = default;

TestExpandoraCommon::~TestExpandoraCommon() = default;

void TestExpandoraCommon::roomCompareTest_data()
{
    QTest::addColumn<QtRoomWrapper>("room");
    QTest::addColumn<ParseEvent>("event");
    QTest::addColumn<ComparisonResultEnum>("comparison");

    const RoomName name{"Riverside"};
    // NOTE: This doesn't contain newlines!?!
    const RoomDesc desc = makeRoomDesc(
        "The high plateau to the north shelters this place from the cold northern winds"
        " during the winter. It would be difficult to climb up there from here, as the"
        " plateau above extends over this area, making a sheltered hollow underneath it"
        " with a depression in the dirt wall at the back. To the east a deep river flows"
        " quickly, while a shallower section lies to the south.");
    const RoomContents contents = makeRoomContents("The corpse of a burly orc is lying here.");
    static constexpr const auto terrain = RoomTerrainEnum::FIELD;
    static constexpr const auto lit = RoomLightEnum::LIT;

    const auto create_room = [&name, &desc, &contents](
                                 const std::function<void(ExternalRawRoom &)> &callback =
                                     [](MAYBE_UNUSED ExternalRawRoom &ignored) {}) -> QtRoomWrapper {
        mmqt::HideQDebug forThisFunction;

        ExternalRawRoom builder;
        builder.id = DEFAULT_EXTERNAL_ROOMID;
        builder.setName(name);
        builder.setDescription(desc);
        builder.setContents(contents);
        builder.setTerrainType(terrain);
        builder.setLightType(lit);
        auto &eList = builder.exits;
        eList[ExitDirEnum::NORTH].setExitFlags(
            ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT | ExitFlagEnum::ROAD});
        eList[ExitDirEnum::SOUTH].setExitFlags(ExitFlags{ExitFlagEnum::EXIT});
        eList[ExitDirEnum::EAST].setExitFlags(ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::EXIT});
        eList[ExitDirEnum::WEST].setExitFlags(ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});
        eList[ExitDirEnum::WEST].setDoorFlags(DoorFlags{DoorFlagEnum::HIDDEN});
        eList[ExitDirEnum::DOWN].setExitFlags(
            ExitFlags{ExitFlagEnum::CLIMB | ExitFlagEnum::DOOR | ExitFlagEnum::EXIT});

        // NOTE: We have to add an outgoing exit here because the map now automatically
        // sets or clears the EXIT flag based on the presence of outgoing exits.
        for (auto &e : eList) {
            if (e.exitIsExit()) {
                e.outgoing.insert(DEFAULT_EXTERNAL_ROOMID);
            }
        }

        callback(builder);
        ProgressCounter pc;
        auto room = Map::fromRooms(pc, {builder}).modified.getRoomHandle(DEFAULT_ROOMID);
        auto shared = std::make_shared<RoomHandle>(room);
        return QtRoomWrapper{shared};
    };

    const auto get_exit_flags = [](const QtRoomWrapper &room) -> ExitsFlagsType {
        return room.shared->computeExitsFlags();
    };

    const auto perfect_room = create_room();
    const auto perfect_exit_flags = get_exit_flags(perfect_room);

    // Blinded / Puke in Darkness
    {
        const auto &room = perfect_room;
        const auto event = createParseEvent(room.getTerrainType());
        QTest::newRow("blinded") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Puke with Awareness in Darkness
    {
        const auto &room = perfect_room;
        const auto event = createParseEvent(name, contents, room.getTerrainType());
        QTest::newRow("awareness") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Whitespace change to room desc
    {
        const auto &room = perfect_room;

        // NOTE: QString::simplified() is roughly equivalent
        // to the sanitizer so this test may not do what you expect anymore.
        const auto event = createParseEvent(name,
                                            mmqt::makeRoomDesc(desc.toQString().simplified()),
                                            contents,
                                            room.getTerrainType());
        QTest::newRow("whitespace") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Single word change to room desc
    {
        const auto &room = perfect_room;
        const auto event = createParseEvent(name,
                                            mmqt::makeRoomDesc(
                                                desc.toQString().replace("difficult", "easy")),
                                            contents,
                                            room.getTerrainType());
        QTest::newRow("single word") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Different room name
    {
        const auto &room = perfect_room;
        const auto event = createParseEvent(RoomName("Riverbank"),
                                            desc,
                                            contents,
                                            room.getTerrainType());
        QTest::newRow("different room name") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Different terrain type
    {
        const auto &room = perfect_room;
        const auto event = createParseEvent(name, desc, contents);
        QTest::newRow("different terrain") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Doors closed hiding exit w
    {
        const auto &room = perfect_room;
        auto exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{}); // Remove door and exit flag to the west
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags);
        QTest::newRow("doors closed") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Doors open hiding climb
    {
        const auto &room = perfect_room;
        auto exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::DOWN,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT}); // Remove climb down
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags);
        QTest::newRow("doors opens") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Sunlight hiding road
    {
        const auto &room = perfect_room;
        auto exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::NORTH,
                      ExitFlags{ExitFlagEnum::DOOR | ExitFlagEnum::EXIT}); // Remove road north
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setDirectSunlight(ExitDirEnum::NORTH, DirectSunlightEnum::SAW_DIRECT_SUN);
        connectedFlags.setDirectSunlight(ExitDirEnum::EAST, DirectSunlightEnum::SAW_DIRECT_SUN);
        connectedFlags.setDirectSunlight(ExitDirEnum::WEST, DirectSunlightEnum::SAW_DIRECT_SUN);
        connectedFlags.setDirectSunlight(ExitDirEnum::DOWN, DirectSunlightEnum::SAW_DIRECT_SUN);
        connectedFlags.setValid();
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags,
                                            connectedFlags);
        QTest::newRow("sunlight hiding road") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Missing exit south
    {
        const auto &room = perfect_room;
        auto exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::SOUTH, ExitFlags{}); // Exit is missing from event
        const auto &south = room.getExit(ExitDirEnum::SOUTH);
        QVERIFY(south.exitIsExit());
        QVERIFY(!south.exitIsDoor());
        QVERIFY(!south.doorIsHidden());
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags);
        QTest::newRow("missing exit south") << room << event << ComparisonResultEnum::DIFFERENT;
    }

    // Missing door and exit down
    {
        const auto &room = perfect_room;
        auto exitFlags = get_exit_flags(room);
        exitFlags.set(ExitDirEnum::DOWN, ExitFlags{}); // Exit and door are missing from event
        const auto &down = room.getExit(ExitDirEnum::DOWN);
        QVERIFY(down.exitIsExit());
        QVERIFY(down.exitIsDoor());
        QVERIFY(!down.doorIsHidden());
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags);
        QTest::newRow("missing door and exit down")
            << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Missing climb e
    {
        const auto &room = perfect_room;
        auto exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT}); // Remove climb e
        const auto &east = room.getExit(ExitDirEnum::EAST);
        QVERIFY(east.exitIsExit());
        QVERIFY(east.exitIsClimb()); // Room has climb e
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags);
        QTest::newRow("missing climb e") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Missing road e
    {
        const auto room = create_room([](ExternalRawRoom &r) {
            //
            r.setExitFlags(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::EXIT});
        });
        auto exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::EAST,
                      ExitFlags{ExitFlagEnum::ROAD | ExitFlagEnum::EXIT}); // Event has road e

        const auto &east = room.getExit(ExitDirEnum::EAST);
        QVERIFY(east.exitIsExit());
        QVERIFY(!east.exitIsRoad()); // Room has no road e
        const auto event = createParseEvent(CommandEnum::NORTH,
                                            name,
                                            desc,
                                            contents,
                                            room.getTerrainType(),
                                            exitFlags,
                                            ConnectedRoomFlagsType{});
        QTest::newRow("missing road n") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Door west not marked as hidden
    {
        const auto room = create_room([](ExternalRawRoom &r) {
            auto &west = r.exits[ExitDirEnum::WEST];
            west.setDoorFlags(west.getDoorFlags() ^ DoorFlagEnum::HIDDEN);
        });
        ExitsFlagsType exitFlags = perfect_exit_flags;
        exitFlags.set(ExitDirEnum::WEST, ExitFlags{}); // Exit and door are missing from event
        const auto &west = room.getExit(ExitDirEnum::WEST);
        QVERIFY(west.exitIsExit());
        QVERIFY(west.exitIsDoor());
        QVERIFY(!west.doorIsHidden());
        auto event = createParseEvent(CommandEnum::NORTH,
                                      name,
                                      desc,
                                      contents,
                                      room.getTerrainType(),
                                      exitFlags);
        QTest::newRow("door west not hidden") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Outdated and no exits (likely player generated room)
    {
        const auto room = create_room([](ExternalRawRoom &r) { r.exits = {}; });
        ExitsFlagsType exitFlags = get_exit_flags(room);

        QVERIFY(!room.getExit(ExitDirEnum::SOUTH).exitIsExit());
        auto event = createParseEvent(CommandEnum::NORTH,
                                      name,
                                      desc,
                                      contents,
                                      room.getTerrainType(),
                                      exitFlags);
        QTest::newRow("outdated and no exits") << room << event << ComparisonResultEnum::EQUAL;
    }

    // Lit
    {
        const auto room = create_room([](ExternalRawRoom &r) {
            r.setLightType(RoomLightEnum::UNDEFINED);
            r.setSundeathType(RoomSundeathEnum::NO_SUNDEATH);
        });
        PromptFlagsType promptFlags{};
        promptFlags.setLit();
        promptFlags.setValid();
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setValid();
        connectedFlags.setTrollMode();
        auto event = createParseEvent(CommandEnum::NORTH,
                                      name,
                                      desc,
                                      contents,
                                      room.getTerrainType(),
                                      get_exit_flags(room),
                                      promptFlags,
                                      connectedFlags);
        QTest::newRow("lit") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // Dark
    {
        const auto room = create_room([](ExternalRawRoom &r) {
            r.setLightType(RoomLightEnum::UNDEFINED);
            r.setSundeathType(RoomSundeathEnum::NO_SUNDEATH);
        });
        PromptFlagsType promptFlags{};
        promptFlags.setDark();
        promptFlags.setValid();
        ConnectedRoomFlagsType connectedFlags;
        connectedFlags.setDirectSunlight(ExitDirEnum::NORTH, DirectSunlightEnum::SAW_DIRECT_SUN);
        connectedFlags.setValid();
        connectedFlags.setTrollMode();
        auto event = createParseEvent(CommandEnum::NORTH,
                                      name,
                                      desc,
                                      contents,
                                      room.getTerrainType(),
                                      get_exit_flags(room),
                                      promptFlags,
                                      connectedFlags);
        QTest::newRow("dark") << room << event << ComparisonResultEnum::TOLERANCE;
    }

    // REVISIT: More negative cases
}

void TestExpandoraCommon::roomCompareTest()
{
    QFETCH(QtRoomWrapper, room);
    QFETCH(ParseEvent, event);
    QFETCH(ComparisonResultEnum, comparison);

    const auto result = [&room, &event]() {
        // REVISIT: Config has default matching tolerance of 8
        const int matchingTolerance = 8;
        mmqt::HideQDebug forThisFunctionCall; // e.g. "Updating room to be LIT"
        return ::compare(room.shared->getRaw(), event, matchingTolerance);
    }();

    if (comparison != result) {
        auto temp = createTemporaryRoom(event);
        qDebug() << "Expected" << to_str(comparison) << "but got" << to_str(result) << "\n"
                 << mmqt::toQStringUtf8(room.shared->toStdStringUtf8()) << "\n"
                 << mmqt::toQStringUtf8(temp.toStdStringUtf8());
    }

    QCOMPARE(result, comparison);
}

QTEST_MAIN(TestExpandoraCommon)
