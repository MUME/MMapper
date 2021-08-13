#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "OpenGLProber.h"

#include <string>

namespace OpenGLConfig {

NODISCARD extern OpenGLProber::BackendType getBackendType();
extern void setBackendType(OpenGLProber::BackendType type);

NODISCARD extern bool getIsCompat();
extern void setIsCompat(bool isCompat);

NODISCARD extern std::string getHighestReportableVersionString();
extern void setHighestReportableVersionString(const std::string &versionString);

} // namespace OpenGLConfig
