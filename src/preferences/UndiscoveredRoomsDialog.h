#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

class QMessageBox;
class QWidget;

// Non-blocking, parent-owned confirmation shared by preferences and character login.
// On OK, remembers "Don't show again"; callers handle enabling or cancelling the overlay.
QMessageBox &showUndiscoveredRoomsDialog(QWidget *parent);
