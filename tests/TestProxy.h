#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestProxy final : public QObject
{
    Q_OBJECT

public:
    TestProxy() = default;
    ~TestProxy() override = default;

private Q_SLOTS:
    void escapeTest();
    void gmcpMessageDeserializeTest();
    void gmcpMessageSerializeTest();
    void gmcpModuleTest();
};
