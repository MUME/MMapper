#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ConfigEnums.h"

#include <stdexcept>

#include <qprocessordetection.h>
#include <qsystemdetection.h>

static inline constexpr const PlatformEnum CURRENT_PLATFORM = [] {
#if defined(Q_OS_WIN)
    return PlatformEnum::Windows;
#elif defined(Q_OS_MAC)
    return PlatformEnum::Mac;
#elif defined(Q_OS_LINUX)
    return PlatformEnum::Linux;
#elif defined(Q_OS_WASM)
    return PlatformEnum::Wasm;
#else
    throw std::runtime_error("unsupported platform");
#endif
}();

static inline constexpr const EnvironmentEnum CURRENT_ENVIRONMENT = [] {
#if Q_PROCESSOR_WORDSIZE == 4
    return EnvironmentEnum::Env32Bit;
#elif Q_PROCESSOR_WORDSIZE == 8
    return EnvironmentEnum::Env64Bit;
#else
    throw std::runtime_error("unsupported environment");
#endif
}();
