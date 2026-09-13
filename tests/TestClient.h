#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../src/global/macros.h"

#include <QObject>

class NODISCARD_QOBJECT TestClient final : public QObject
{
    Q_OBJECT

public:
    TestClient();
    ~TestClient() final;

private Q_SLOTS:
    // InputHistory
    void inputHistoryDedupVsBackOnly();
    void inputHistoryEmptyLineSkipped();
    void inputHistoryCap();
    void inputHistoryNavigation();

    // TabHistory
    void tabHistoryWordExtraction();
    void tabHistoryCap();
    void tabHistoryNextMatchCycling();

    // RemoteEditDocumentOps (widget-free core of the MPI remote editor)
    void remoteEditJustifyTextRewrapsLongLines();
    void remoteEditExpandTabsExpandsSelection();
    void remoteEditRemoveDuplicateSpacesCollapses();
    void remoteEditJoinLinesCombinesBlocks();
    void remoteEditPrefixPartialSelectionQuotesLines();
    void remoteEditFindWrapsAround();
    void remoteEditReplaceAllReplacesEveryOccurrence();
    void remoteEditStatusReportsTabsLongLinesAndTrailingSpace();
};
