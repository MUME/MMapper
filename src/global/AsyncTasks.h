#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "progresscounter.h"

#include <functional>
#include <string>

namespace async_tasks {
using BackgroundWorker = std::function<void(ProgressCounter &pc)>;
using OnSuccess = std::function<void()>;

extern void init();
extern void cleanup();
extern void startAsyncTask(std::string name, BackgroundWorker backgroundWorker, OnSuccess onSuccess);
} // namespace async_tasks
