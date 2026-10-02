#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestRemoteEditDraftStore final : public QObject
{
    Q_OBJECT

public:
    TestRemoteEditDraftStore();
    ~TestRemoteEditDraftStore() final;

private Q_SLOTS:
    void testRoundTrip_data();
    void testRoundTrip();
    void testCollidingSessionIds_data();
    void testCollidingSessionIds();
    void testRemoveUnknownKeyIsHarmless_data();
    void testRemoveUnknownKeyIsHarmless();
    void testFileStoreKeepsTitleWithAwkwardCharacters();
    void testFileStoreIgnoresForeignFiles();
    void testFileStoreExposesPath();
    void testSettingsStoreHasNoPath();
};
