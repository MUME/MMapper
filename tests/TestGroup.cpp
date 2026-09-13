// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestGroup.h"

#include "../src/configuration/configuration.h"
#include "../src/group/CGroupChar.h"
#include "../src/group/GroupModel.h"
#include "../src/group/enums.h"

#include <QDebug>
#include <QtTest/QtTest>

TestGroup::TestGroup()
{
    // GroupModel::insertCharacter()/setCharacters() read getConfig(), which
    // asserts that setEnteredMain() has run first (see configuration.cpp).
    setEnteredMain();
}
TestGroup::~TestGroup() = default;

void TestGroup::enumsTest()
{
    test::test_group_enums();
}

void TestGroup::roleNamesTest()
{
    GroupModel model;
    // rowCount()/data()/roleNames() are protected on GroupModel (only Qt's view
    // internals call them, via a QAbstractItemModel* the same way GroupWidget's
    // QTableView does); reach them through the public base-class interface.
    QAbstractItemModel &base = model;
    const QHash<int, QByteArray> roles = base.roleNames();
    const QList<QByteArray> names = roles.values();

    for (const char *expected : {"name",
                                 "charColor",
                                 "textColor",
                                 "hpText",
                                 "manaText",
                                 "movesText",
                                 "hpRatio",
                                 "manaRatio",
                                 "movesRatio",
                                 "hpLow",
                                 "movesLow",
                                 "manaHidden",
                                 "stateIcons",
                                 "stateTip",
                                 "roomName",
                                 "isYou",
                                 "isNpc",
                                 "canCenter"}) {
        QVERIFY2(names.contains(QByteArray(expected)), expected);
    }
}

void TestGroup::characterRoleDataTest()
{
    GroupModel model;
    QAbstractItemModel &base = model;

    SharedGroupChar character = CGroupChar::alloc();
    character->setId(GroupId{1});
    character->setColor(QColor(Qt::black));
    character->setPosition(CharacterPositionEnum::STANDING);
    character->setScore(/*hp=*/20,
                        /*maxhp=*/100,
                        /*mana=*/0,
                        /*maxmana=*/0,
                        /*moves=*/10,
                        /*maxmoves=*/100);

    model.insertCharacter(character);
    QCOMPARE(base.rowCount(QModelIndex()), 1);

    const QModelIndex idx = base.index(0, 0);
    QVERIFY(idx.isValid());

    QCOMPARE(base.data(idx, GroupModel::HpRatioRole).toDouble(), 0.20);
    QVERIFY(base.data(idx, GroupModel::HpLowRole).toBool());
    QVERIFY(base.data(idx, GroupModel::MovesLowRole).toBool());
    QVERIFY(base.data(idx, GroupModel::ManaHiddenRole).toBool());
    QCOMPARE(base.data(idx, GroupModel::ManaTextRole).toString(), QStringLiteral("--"));
    QVERIFY(!base.data(idx, GroupModel::HpTextRole).toString().isEmpty());
    QVERIFY(!base.data(idx, GroupModel::StateIconsRole).toStringList().isEmpty());

    // anyMana stays false since the only character has no mana pool.
    QVERIFY(!model.getAnyMana());

    // A character with unknown maxima (max <= 0) must not report low stats,
    // matching GroupDelegate's pulse decision for unknown state.
    SharedGroupChar unknown = CGroupChar::alloc();
    unknown->setId(GroupId{2});
    unknown->setColor(QColor(Qt::white));
    unknown->setScore(/*hp=*/0, /*maxhp=*/0, /*mana=*/0, /*maxmana=*/0, /*moves=*/0, /*maxmoves=*/0);
    model.insertCharacter(unknown);
    const QModelIndex idx2 = base.index(1, 0);
    QVERIFY(!base.data(idx2, GroupModel::HpLowRole).toBool());
    QVERIFY(!base.data(idx2, GroupModel::MovesLowRole).toBool());
}

void TestGroup::moveRowTest()
{
    GroupModel model;
    QAbstractItemModel &base = model;
    for (uint32_t i = 1; i <= 3; ++i) {
        SharedGroupChar character = CGroupChar::alloc();
        character->setId(GroupId{i});
        model.insertCharacter(character);
    }
    QCOMPARE(base.rowCount(QModelIndex()), 3);

    // Out-of-bounds and no-op moves are rejected.
    QVERIFY(!model.moveRow(-1, 0));
    QVERIFY(!model.moveRow(0, 100));
    QVERIFY(!model.moveRow(0, 0));
    QVERIFY(!model.moveRow(0, 1));

    QVERIFY(model.moveRow(0, 2));
    QCOMPARE(model.getCharacter(0)->getId(), GroupId{2});
    QCOMPARE(model.getCharacter(1)->getId(), GroupId{1});
    QCOMPARE(model.getCharacter(2)->getId(), GroupId{3});
}

QTEST_MAIN(TestGroup)
