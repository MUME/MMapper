#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "macros.h"

#ifndef NDEBUG
static constexpr const bool IS_DEBUG_BUILD = true;
#else
static constexpr const bool IS_DEBUG_BUILD = false;
#endif

#if defined(MMAPPER_NO_OPENSSL) && MMAPPER_NO_OPENSSL
static inline constexpr const bool NO_OPEN_SSL = true;
#else
static inline constexpr const bool NO_OPEN_SSL = false;
#endif

#if defined(MMAPPER_NO_UPDATER) && MMAPPER_NO_UPDATER
static inline constexpr const bool NO_UPDATER = true;
#else
static inline constexpr const bool NO_UPDATER = false;
#endif

#if defined(MMAPPER_NO_MAP) && MMAPPER_NO_MAP
static inline constexpr const bool NO_MAP_RESOURCE = true;
#else
static inline constexpr const bool NO_MAP_RESOURCE = false;
#endif

#if defined(MMAPPER_NO_ZLIB) && MMAPPER_NO_ZLIB
static inline constexpr const bool NO_ZLIB = true;
#else
static inline constexpr const bool NO_ZLIB = false;
#endif
