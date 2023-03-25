#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "adventure/lineparsers.h"
#include <QObject>

struct TestLine
{
    bool expected;
    QString line;

    QString errorMsg()
    {
        return QString("Parsing failed, expected %1 for line: %2").arg(expected).arg(line);
    }
};

class TestAdventure final : public QObject
{
    Q_OBJECT

    void testParser(AbstractLineParser &parser, std::vector<TestLine> testLines);

private slots:

    void testSessionHourlyRateXP();

    void testAccomplishedTaskParser();
    void testAchievementParser();
    void testHintParser();
    void testKillAndXPParser();
};
