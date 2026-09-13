#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestMedia final : public QObject
{
    Q_OBJECT

public:
    TestMedia();
    ~TestMedia() final;

private Q_SLOTS:
    void stackBlurPreservesSize();
    void stackBlurChangesGradient();
    void stackBlurZeroRadiusIsNoop();
    void stackBlurHandlesTinyImage();
};
