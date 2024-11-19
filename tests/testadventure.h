#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "../src/adventure/lineparsers.h"
#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestAdventure final : public QObject
{
    Q_OBJECT

private slots:
    static void testSessionHourlyRateXP();
    static void testAchievementParser();
    static void testHintParser();
    static void testKillAndXPParser();
    static void testE2E();
};
