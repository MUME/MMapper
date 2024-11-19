#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include <memory>

class QString;
class QMainWindow;

extern void initTopLevelWindows();
extern void destroyTopLevelWindows();
extern void addTopLevelWindow(std::unique_ptr<QMainWindow> window);
