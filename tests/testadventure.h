#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include <QObject>

class TestAdventure final : public QObject
{
    Q_OBJECT

public:
    TestAdventure();
    ~TestAdventure() final;

private Q_SLOTS:

    void testSessionHourlyRateXP();

    void testAccomplishedTaskParser();
    void testAchievementParser();
    void testDiedParser();
    void testGainedLevelParser();
    void testHintParser();
    void testKillAndXPParser();
};
