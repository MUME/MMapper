#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ConfigEnums.h"

#include <stdexcept>
#include <string_view>

#include <qprocessordetection.h>
#include <qsystemdetection.h>

static inline constexpr PackageEnum CURRENT_PACKAGE = [] {
    using namespace std::string_view_literals;
    constexpr auto type = MMAPPER_PACKAGE_TYPE;
    // FIXME: Use XFOREACH macro
    if constexpr (type == "Source"sv) {
        return PackageEnum::Source;
    }
    if constexpr (type == "Deb"sv) {
        return PackageEnum::Deb;
    }
    if constexpr (type == "Dmg"sv) {
        return PackageEnum::Dmg;
    }
    if constexpr (type == "Nsis"sv) {
        return PackageEnum::Nsis;
    }
    if constexpr (type == "AppImage"sv) {
        return PackageEnum::AppImage;
    }
    if constexpr (type == "AppX"sv) {
        return PackageEnum::AppX;
    }
    if constexpr (type == "Flatpak"sv) {
        return PackageEnum::Flatpak;
    }
    if constexpr (type == "Snap"sv) {
        return PackageEnum::Snap;
    }
    if constexpr (type == "Wasm"sv) {
        return PackageEnum::Wasm;
    }
    throw std::runtime_error("unsupported package type");
}();

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
