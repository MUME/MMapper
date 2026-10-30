// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestProxy.h"

#include "../src/global/TextUtils.h"
#include "../src/observer/KnownRooms.h"
#include "../src/observer/KnownRoomsCache.h"
#include "../src/observer/gameobserver.h"
#include "../src/proxy/GmcpMessage.h"
#include "../src/proxy/GmcpModule.h"
#include "../src/proxy/GmcpUtils.h"
#include "../src/proxy/telnetfilter.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

void TestProxy::escapeTest()
{
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"(12345)"), QString(R"(12345)"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"(1.0)"), QString(R"(1.0)"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"(true)"), QString(R"(true)"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"("Hello")"), QString(R"(\"Hello\")"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData("\\\n\r\b\f\t"), QString(R"(\\\n\r\b\f\t)"));
}

void TestProxy::gmcpMessageDeserializeTest()
{
    GmcpMessage gmcp1 = GmcpMessage::fromRawBytes(R"(Core.Hello { "Hello": "world" })");
    QCOMPARE(gmcp1.getName().toQByteArray(), QByteArray("Core.Hello"));
    QCOMPARE(gmcp1.getJson()->toQString(), mmqt::toQStringUtf8(R"({ "Hello": "world" })"));

    GmcpMessage gmcp2 = GmcpMessage::fromRawBytes(R"(Core.Goodbye)");
    QCOMPARE(gmcp2.getName().toQByteArray(), QByteArray("Core.Goodbye"));
    QVERIFY(!gmcp2.getJson());

    GmcpMessage gmcp3 = GmcpMessage::fromRawBytes(R"(External.Discord.Hello)");
    QCOMPARE(gmcp3.getName().toQByteArray(), QByteArray("External.Discord.Hello"));
    QVERIFY(!gmcp3.getJson());
}

void TestProxy::gmcpMessageSerializeTest()
{
    GmcpMessage gmcp1(GmcpMessageTypeEnum::CORE_HELLO);
    QCOMPARE(gmcp1.toRawBytes(), QByteArray("Core.Hello"));

    GmcpMessage gmcp2(GmcpMessageTypeEnum::CORE_HELLO, GmcpJson{"{}"});
    QCOMPARE(gmcp2.toRawBytes(), QByteArray("Core.Hello {}"));
}

void TestProxy::gmcpModuleTest()
{
    GmcpModule module1("Char 1");
    QCOMPARE(mmqt::toQByteArrayUtf8(module1.getNormalizedName()), QByteArray("char"));
    QCOMPARE(module1.getVersion().asUint32(), 1u);
    QVERIFY(module1.isSupported());

    GmcpModule module2("Char.Skills 1");
    QCOMPARE(mmqt::toQByteArrayUtf8(module2.getNormalizedName()), QByteArray("char.skills"));
    QCOMPARE(module2.getVersion().asUint32(), 1u);
    QVERIFY(!module2.isSupported());

    GmcpModule module3("Room");
    QCOMPARE(mmqt::toQByteArrayUtf8(module3.getNormalizedName()), QByteArray("room"));
    QCOMPARE(module3.getVersion().asUint32(), 0u);
    QVERIFY(module3.isSupported());
}

void TestProxy::telnetFilterTest()
{
    test::test_telnetfilter();
}

void TestProxy::knownRoomsHistoryTest()
{
    KnownRooms known;
    QVERIFY(!known.setEnabled(true));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{99}));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [1,3,4711]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 42")));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [19,3]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    const auto snapshot = known.getSnapshot();
    QVERIFY(snapshot.complete);
    QCOMPARE(snapshot.rooms.size(), size_t{5});
    QVERIFY(!snapshot.isUndiscovered(ServerRoomId{4711}));
    QVERIFY(!snapshot.isUndiscovered(ServerRoomId{42}));
    QVERIFY(!snapshot.isUndiscovered(INVALID_SERVER_ROOMID));
    QVERIFY(snapshot.isUndiscovered(ServerRoomId{99}));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 99")));
    QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{99}));
    QCOMPARE(known.getSnapshot().generation, snapshot.generation);
    QVERIFY(snapshot.isUndiscovered(ServerRoomId{99})); // immutable worker snapshot
    const auto revision = known.getSnapshot().revision;
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 99")));
    QCOMPARE(known.getSnapshot().revision, revision);
    known.reset();
    QVERIFY(known.getSnapshot().generation != snapshot.generation);
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(known.getSnapshot().rooms.empty());
}

void TestProxy::knownRoomsSwitchTest()
{
    KnownRooms known;
    QVERIFY(!known.setEnabled(true));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [1,2]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Frodo"})")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [3,4]")));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [7]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QCOMPARE(known.getSnapshot().rooms.size(), size_t{1});
    QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{1}));
    QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{7}));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Core.Goodbye")));
    QVERIFY(!known.getSnapshot().complete);
    for (const auto *identity : {"Char.Name null", "Char.Name {}", "Char.Name"}) {
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [1]")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes(identity)));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
        QVERIFY(!known.getSnapshot().complete);
    }
}

void TestProxy::knownRoomsUnavailableTest()
{
    KnownRooms known;
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QVERIFY(known.setEnabled(true)); // can be enabled during play
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List false")));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(known.receive(GmcpMessage::fromRawBytes("Room.Known.Updated")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(known.getSnapshot().complete); // a completed empty list is authoritative
    QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{1}));
    QVERIFY(!known.setEnabled(false));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(known.setEnabled(true));
    QVERIFY(!known.setEnabled(false));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [1]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(known.setEnabled(true));
}

void TestProxy::knownRoomsMalformedTest()
{
    for (const auto *body : {"[1,-2]",
                             "[1,0]",
                             "[1,1.5]",
                             "[1,4294967296]",
                             "[true]",
                             "[null]",
                             "[\"2\"]",
                             "{}",
                             "true",
                             "[broken"}) {
        KnownRooms known;
        QVERIFY(!known.setEnabled(true));
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes(QByteArray("Room.Known.List ") + body)));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
        QVERIFY(!known.getSnapshot().complete);
    }
    KnownRooms known;
    QVERIFY(!known.setEnabled(true));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [4294967295]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    for (const auto *body : {"-1", "0", "1.5", "4294967296", "42junk", "[42]"}) {
        const auto revision = known.getSnapshot().revision;
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes(QByteArray("Room.Known.Add ") + body)));
        QCOMPARE(known.getSnapshot().revision, revision);
    }
    QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{UINT32_MAX}));
    QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{42}));
}

void TestProxy::knownRoomsRefreshTest()
{
    KnownRooms known;
    QVERIFY(!known.setEnabled(true));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [1]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Updated")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Updated")));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [2]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{1}));
    QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{2}));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes("Room.Known.Updated")));
    QVERIFY(!known.getSnapshot().complete);
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [3]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":""})")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(!known.getSnapshot().complete);
}

void TestProxy::knownRoomsObserverTest()
{
    GameObserver observer;
    Signal2Lifetime lifetime;
    int requests = 0;
    int changes = 0;
    observer.sig2_requestKnownRooms.connect(lifetime, [&requests]() { ++requests; });
    observer.sig2_knownRoomsChanged.connect(lifetime, [&changes]() { ++changes; });
    observer.enableKnownRooms(true);
    observer.observeConnected();
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})"));
    QCOMPARE(requests, 1);
    const auto beforeList = changes;
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes("Room.Known.List [1,2]"));
    QCOMPARE(changes, beforeList);
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes("Room.Known.List null"));
    QCOMPARE(changes, beforeList + 1);
    QVERIFY(observer.getKnownRooms().complete);
    observer.resetKnownRooms();
    QVERIFY(!observer.getKnownRooms().complete);
}

namespace {
void seedKnownRooms(const QString &directory, const QByteArray &name, const QByteArray &ids)
{
    KnownRooms known;
    known.configureCache(directory, "mume.org:4242", []() {});
    QVERIFY(!known.setEnabled(true));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes("Char.Name {\"name\":\"" + name + "\"}")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List " + ids)));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
}
} // namespace

void TestProxy::knownRoomsCacheMissingWarningTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1,2]");
    GameObserver observer;
    int warnings = 0;
    Signal2Lifetime lifetime;
    observer.sig2_knownRoomsCacheMissing.connect(lifetime, [&warnings]() { ++warnings; });
    observer.configureKnownRoomsCache(dir.path(), "mume.org:4242");
    observer.enableKnownRooms(true);
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})"));
    QTRY_VERIFY(observer.getKnownRooms().complete);
    QCOMPARE(warnings, 0);
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Frodo"})"));
    QTRY_COMPARE(warnings, 1);
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Frodo"})"));
    QCOMPARE(warnings, 1);
    // A completed server response beats a late missing-cache result.
    observer.resetKnownRooms();
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Sam"})"));
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes("Room.Known.List null"));
    QTest::qWait(100);
    QCOMPARE(warnings, 1);
    observer.resetKnownRooms();
    observer.enableKnownRooms(false);
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Merry"})"));
    QTest::qWait(100);
    QCOMPARE(warnings, 1);
    // A read for a character that has already changed must not warn.
    observer.enableKnownRooms(true);
    observer.resetKnownRooms();
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Pippin"})"));
    observer.observeSentToUserGmcp(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})"));
    QTRY_VERIFY(observer.getKnownRooms().complete);
    QCOMPARE(warnings, 1);
}

void TestProxy::knownRoomsCacheRestartTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1,2]");
    int changed = 0;
    {
        KnownRooms known;
        known.configureCache(dir.path(), "mume.org:4242", [&changed]() { ++changed; });
        QVERIFY(!known.setEnabled(true));
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QTRY_VERIFY(known.getSnapshot().complete);
        QVERIFY(changed > 0);
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{1}));
        // Keep the cached view during the slow refresh, then allow truth to remove stale IDs.
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [2,3]")));
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{1}));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
        QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{1}));
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{3}));
    }
    KnownRooms next;
    next.configureCache(dir.path(), "mume.org:4242", []() {});
    QVERIFY(!next.setEnabled(true));
    QVERIFY(next.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QTRY_VERIFY(next.getSnapshot().complete);
    QVERIFY(next.getSnapshot().isUndiscovered(ServerRoomId{1}));
    QVERIFY(!next.getSnapshot().isUndiscovered(ServerRoomId{3}));
}

void TestProxy::knownRoomsCacheLiveTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1,2]");
    {
        KnownRooms known;
        known.configureCache(dir.path(), "mume.org:4242", []() {});
        QVERIFY(!known.setEnabled(true));
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        // A discovery arriving before the disk read completes must not be lost.
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 9")));
        QTRY_VERIFY(known.getSnapshot().complete);
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{9}));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [1]")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 10")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{9}));
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{10}));
        QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{2}));
        QVERIFY(!known.setEnabled(false));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 11")));
        // Continuous updates are coalesced; ordinary shutdown still saves the last one.
        for (int id = 20; id < 120; ++id) {
            QVERIFY(!known.receive(
                GmcpMessage::fromRawBytes("Room.Known.Add " + QByteArray::number(id))));
        }
    }
    KnownRooms next;
    next.configureCache(dir.path(), "mume.org:4242", []() {});
    QVERIFY(!next.setEnabled(true));
    QVERIFY(next.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QTRY_VERIFY(next.getSnapshot().complete);
    QCOMPARE(next.getSnapshot().rooms.size(), size_t{104});
    QVERIFY(!next.getSnapshot().isUndiscovered(ServerRoomId{11}));
    QVERIFY(!next.getSnapshot().isUndiscovered(ServerRoomId{119}));
}

void TestProxy::knownRoomsCacheInterruptedTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1,2]");
    {
        KnownRooms known;
        known.configureCache(dir.path(), "mume.org:4242", []() {});
        QVERIFY(!known.setEnabled(true));
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QTRY_VERIFY(known.getSnapshot().complete);
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [3]")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List false")));
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{1}));
        QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{3}));
    }
    KnownRooms next;
    next.configureCache(dir.path(), "mume.org:4242", []() {});
    QVERIFY(!next.setEnabled(true));
    QVERIFY(next.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QTRY_VERIFY(next.getSnapshot().complete);
    QCOMPARE(next.getSnapshot().rooms.size(), size_t{2});
}

void TestProxy::knownRoomsCacheIdentityTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1]");
    seedKnownRooms(dir.path(), "Frodo", "[2]");
    KnownRooms known;
    known.configureCache(dir.path(), "mume.org:4242", []() {});
    QVERIFY(!known.setEnabled(true));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Frodo"})")));
    QTRY_VERIFY(known.getSnapshot().complete);
    QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{1}));
    QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{2}));
    QVERIFY(known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [2]")));
    QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
    known.reset();
    QVERIFY(!known.getSnapshot().complete);
    bool done = false;
    bool hit = true;
    KnownRoomsCache other(dir.path(),
                          "another-server:4242",
                          [&done, &hit](const QString &,
                                        uint64_t,
                                        const std::optional<KnownRoomsCache::Rooms> &rooms) {
                              done = true;
                              hit = rooms.has_value();
                          });
    other.load("Bilbo", 1);
    QTRY_VERIFY(done);
    QVERIFY(!hit);
}

void TestProxy::knownRoomsCacheServerSwitchTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1,2]");
    {
        KnownRooms known;
        known.configureCache(dir.path(), "mume.org:4242", []() {});
        QVERIFY(!known.setEnabled(true));
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QTRY_VERIFY(known.getSnapshot().complete);
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 3")));
        // Replacing a server retains its pending I/O and final updates until teardown.
        known.configureCache(dir.path(), "other.example:4242", []() {});
        QVERIFY(!known.getSnapshot().complete);
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [99]")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
        // Immediately returning must read the original server's queued newer snapshot.
        known.configureCache(dir.path(), "mume.org:4242", []() {});
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        QTRY_VERIFY(known.getSnapshot().complete);
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{3}));
        QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{99}));
    }
    KnownRooms original;
    original.configureCache(dir.path(), "mume.org:4242", []() {});
    QVERIFY(!original.setEnabled(true));
    QVERIFY(original.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QTRY_VERIFY(original.getSnapshot().complete);
    QCOMPARE(original.getSnapshot().rooms.size(), size_t{3});
    QVERIFY(!original.getSnapshot().isUndiscovered(ServerRoomId{3}));
    QVERIFY(original.getSnapshot().isUndiscovered(ServerRoomId{99}));
}

void TestProxy::knownRoomsCacheCaseFoldTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    KnownRoomsCache::Rooms first;
    first.insert(ServerRoomId{1});
    auto latest = first;
    latest.insert(ServerRoomId{2});
    bool loaded = false;
    KnownRoomsCache cache(dir.path(),
                          "mume.org:4242",
                          [&loaded, &latest](const QString &,
                                             uint64_t,
                                             const std::optional<KnownRoomsCache::Rooms> &rooms) {
                              QVERIFY(rooms.has_value());
                              QVERIFY(*rooms == latest);
                              loaded = true;
                          });
    // Both spellings address the same file and must share the same pending write.
    cache.save("Bilbo", first);
    cache.save("bilbo", latest);
    cache.load("BILBO", 1);
    QTRY_VERIFY(loaded);
}

void TestProxy::knownRoomsCacheCorruptTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1]");
    const auto files = QDir(dir.path()).entryList({"*.json"}, QDir::Files);
    QCOMPARE(files.size(), qsizetype{1});
    for (const auto &contents :
         {QByteArray("{broken"),
          QByteArray(R"({"version":2,"server":"mume.org:4242","character":"bilbo","rooms":[1]})"),
          QByteArray(R"({"version":1,"server":"mume.org:4242","character":"frodo","rooms":[1]})"),
          QByteArray(R"({"version":1,"server":"mume.org:4242","character":"bilbo","rooms":[-1]})")}) {
        QFile file(QDir(dir.path()).filePath(files.front()));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(contents), qint64{contents.size()});
        file.close();
        bool done = false;
        bool hit = true;
        KnownRoomsCache cache(dir.path(),
                              "mume.org:4242",
                              [&done, &hit](const QString &,
                                            uint64_t,
                                            const std::optional<KnownRoomsCache::Rooms> &rooms) {
                                  done = true;
                                  hit = rooms.has_value();
                              });
        cache.load("Bilbo", 1);
        QTRY_VERIFY(done);
        QVERIFY(!hit);
    }
}

void TestProxy::knownRoomsCacheTruthWinsTest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    seedKnownRooms(dir.path(), "Bilbo", "[1,2]");
    {
        KnownRooms known;
        known.configureCache(dir.path(), "mume.org:4242", []() {});
        QVERIFY(!known.setEnabled(true));
        QVERIFY(known.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
        // No event-loop turn yet: the cache read cannot have notified the observer.
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List [3]")));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.List null")));
        QTest::qWait(100);
        QVERIFY(known.getSnapshot().isUndiscovered(ServerRoomId{1}));
        QVERIFY(!known.getSnapshot().isUndiscovered(ServerRoomId{3}));
        // Check a normal background save before shutdown, not only destructor flushing.
        const auto files = QDir(dir.path()).entryList({"*.json"}, QDir::Files);
        QCOMPARE(files.size(), qsizetype{1});
        const auto containsId = [&dir, &files](const int id) {
            QFile file(QDir(dir.path()).filePath(files.front()));
            if (!file.open(QIODevice::ReadOnly)) {
                return false;
            }
            return QJsonDocument::fromJson(file.readAll())
                .object()
                .value("rooms")
                .toArray()
                .contains(QJsonValue(id));
        };
        QTRY_VERIFY(containsId(3));
        QVERIFY(!known.receive(GmcpMessage::fromRawBytes("Room.Known.Add 4")));
        QTRY_VERIFY(containsId(4));
    }
    KnownRooms next;
    next.configureCache(dir.path(), "mume.org:4242", []() {});
    QVERIFY(!next.setEnabled(true));
    QVERIFY(next.receive(GmcpMessage::fromRawBytes(R"(Char.Name {"name":"Bilbo"})")));
    QTRY_VERIFY(next.getSnapshot().complete);
    QCOMPARE(next.getSnapshot().rooms.size(), size_t{2});
    QVERIFY(!next.getSnapshot().isUndiscovered(ServerRoomId{4}));
}

QTEST_MAIN(TestProxy)
