// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors
#pragma once

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestGroup final : public QObject
{
    Q_OBJECT

public:
    TestGroup();
    ~TestGroup() final;

private Q_SLOTS:
    static void enumsTest();
};
