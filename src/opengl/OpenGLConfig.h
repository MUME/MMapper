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

NODISCARD extern std::string getGLVersionString();
extern void setGLVersionString(const std::string &versionString);

NODISCARD std::string getESVersionString();
void setESVersionString(const std::string &versionString);

NODISCARD extern int getMaxSamples();
extern void setMaxSamples(int maxSamples);

} // namespace OpenGLConfig
