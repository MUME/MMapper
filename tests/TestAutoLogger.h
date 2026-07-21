#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestAutoLogger final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void getFileTimeFallsBackToLastModified();
    void getFileTimeOnRealFile();
    void keepForeverSelectsNothing();
    void deleteDaysUsesFallbackTime();
    void deleteSizeKeepsNewest();
    void deleteSizeSingleOversizedFile();
};
