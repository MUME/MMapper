#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "../src/adventure/lineparsers.h"
#include "../src/global/macros.h"

#include <QObject>

struct NODISCARD TestLine final
{
    bool expected = false;
    QString line;

    NODISCARD QString errorMsg() const
    {
        return QString("Parsing failed, expected %1 for line: %2").arg(expected).arg(line);
    }
};

class NODISCARD_QOBJECT TestAdventure final : public QObject
{
    Q_OBJECT

    void testParser(AbstractLineParser &parser, const std::vector<TestLine> &testLines);

private slots:

    void testSessionHourlyRateXP();

    void testAchievementParser();
    void testHintParser();
    void testKillAndXPParser();

    void testE2E();
};
