// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestRemoteEditDraftStore.h"

#include "../src/mpi/RemoteEditDraftStore.h"

#include <memory>

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

enum class NODISCARD StoreKind { File, Settings };

struct NODISCARD Fixture final
{
    QTemporaryDir dir;
    std::unique_ptr<RemoteEditDraftStore> store;

    explicit Fixture(const StoreKind kind)
    {
        switch (kind) {
        case StoreKind::File:
            store = std::make_unique<RemoteEditFileDraftStore>(dir.filePath("drafts"));
            break;
        case StoreKind::Settings: {
            const QString ini = dir.filePath("drafts.ini");
            store = std::make_unique<RemoteEditSettingsDraftStore>(
                [ini]() { return std::make_unique<QSettings>(ini, QSettings::IniFormat); });
            break;
        }
        }
    }
};

void addStoreKinds()
{
    QTest::addColumn<int>("kind");
    QTest::newRow("file") << static_cast<int>(StoreKind::File);
    QTest::newRow("settings") << static_cast<int>(StoreKind::Settings);
}

NODISCARD StoreKind fetchKind()
{
    QFETCH(int, kind);
    return static_cast<StoreKind>(kind);
}

} // namespace

TestRemoteEditDraftStore::TestRemoteEditDraftStore() = default;
TestRemoteEditDraftStore::~TestRemoteEditDraftStore() = default;

void TestRemoteEditDraftStore::testRoundTrip_data()
{
    addStoreKinds();
}

void TestRemoteEditDraftStore::testRoundTrip()
{
    Fixture f(fetchKind());
    auto &store = *f.store;

    QVERIFY(store.list().isEmpty());

    const QString key = store.create(RemoteSessionId{42}, "Board post", "first\n");
    QVERIFY(!key.isEmpty());
    QCOMPARE(store.read(key), QString("first\n"));

    const auto listed = store.list();
    QCOMPARE(listed.size(), 1);
    QCOMPARE(listed.front().key, key);
    QCOMPARE(listed.front().title, QString("Board post"));
    QCOMPARE(listed.front().sessionId, RemoteSessionId{42});
    QVERIFY(listed.front().lastModified.isValid());

    QVERIFY(store.save(key, "second\n"));
    QCOMPARE(store.read(key), QString("second\n"));

    store.remove(key);
    QVERIFY(store.list().isEmpty());
    QVERIFY(store.read(key).isEmpty());
}

void TestRemoteEditDraftStore::testCollidingSessionIds_data()
{
    addStoreKinds();
}

void TestRemoteEditDraftStore::testCollidingSessionIds()
{
    // MUME reuses session ids, so two edits with the same id and title must
    // never share a draft.
    Fixture f(fetchKind());
    auto &store = *f.store;

    const QString a = store.create(RemoteSessionId{7}, "Same", "A");
    const QString b = store.create(RemoteSessionId{7}, "Same", "B");
    QVERIFY(a != b);
    QCOMPARE(store.read(a), QString("A"));
    QCOMPARE(store.read(b), QString("B"));
    QCOMPARE(store.list().size(), 2);
}

void TestRemoteEditDraftStore::testRemoveUnknownKeyIsHarmless_data()
{
    addStoreKinds();
}

void TestRemoteEditDraftStore::testRemoveUnknownKeyIsHarmless()
{
    Fixture f(fetchKind());
    auto &store = *f.store;
    const QString key = store.create(RemoteSessionId{1}, "Keep", "x");
    store.remove("");
    store.remove("does-not-exist");
    QCOMPARE(store.list().size(), 1);
    QVERIFY(!store.save("does-not-exist", "y"));
    QCOMPARE(store.read(key), QString("x"));
}

void TestRemoteEditDraftStore::testFileStoreKeepsTitleWithAwkwardCharacters()
{
    Fixture f(StoreKind::File);
    auto &store = *f.store;

    // Slashes, spaces, and a title long enough that the 50-byte cap lands
    // inside a percent-escape; the file name must stay valid and the title
    // must decode to a prefix of the original rather than garbage.
    const QString title = QString("a/b c%d ") + QString(30, QChar(0xE9)) + "tail";
    const QString key = store.create(RemoteSessionId{3}, title, "body");
    QVERIFY(!key.isEmpty());
    QVERIFY(!key.contains('/'));
    QVERIFY(QFile::exists(store.filePath(key)));

    const auto listed = store.list();
    QCOMPARE(listed.size(), 1);
    QVERIFY(title.startsWith(listed.front().title));
    QVERIFY(listed.front().title.startsWith("a/b c%d "));
}

void TestRemoteEditDraftStore::testFileStoreIgnoresForeignFiles()
{
    Fixture f(StoreKind::File);
    auto &store = *f.store;
    std::ignore = store.create(RemoteSessionId{1}, "Real", "x");

    const QDir dir(f.dir.filePath("drafts"));
    for (const char *const name : {"notes.txt", "draft_bogus.txt", "draft_1_x_y.txt"}) {
        QFile file(dir.absoluteFilePath(name));
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("junk");
    }

    const auto listed = store.list();
    QCOMPARE(listed.size(), 1);
    QCOMPARE(listed.front().title, QString("Real"));
}

void TestRemoteEditDraftStore::testFileStoreExposesPath()
{
    Fixture f(StoreKind::File);
    const QString key = f.store->create(RemoteSessionId{1}, "T", "x");
    QVERIFY(f.store->filePath(key).startsWith(f.dir.path()));
    QVERIFY(f.store->filePath("").isEmpty());
}

void TestRemoteEditDraftStore::testSettingsStoreHasNoPath()
{
    Fixture f(StoreKind::Settings);
    const QString key = f.store->create(RemoteSessionId{1}, "T", "x");
    QVERIFY(f.store->filePath(key).isEmpty());

    // A fresh store over the same settings sees the draft: this is the
    // "survives a reload" property the WebAssembly build relies on.
    const QString ini = f.dir.filePath("drafts.ini");
    RemoteEditSettingsDraftStore reopened(
        [ini]() { return std::make_unique<QSettings>(ini, QSettings::IniFormat); });
    QCOMPARE(reopened.list().size(), 1);
    QCOMPARE(reopened.read(key), QString("x"));
}

QTEST_MAIN(TestRemoteEditDraftStore)
