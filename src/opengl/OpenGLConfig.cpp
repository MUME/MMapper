// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "OpenGLConfig.h"

namespace OpenGLConfig {

static OpenGLProber::BackendType g_backendType;
static bool g_isCompat;
static std::string g_versionString;

NODISCARD OpenGLProber::BackendType getBackendType()
{
    return g_backendType;
}

void setBackendType(OpenGLProber::BackendType type)
{
    g_backendType = type;
}

NODISCARD bool getIsCompat()
{
    return g_isCompat;
}

void setIsCompat(bool isCompat)
{
    g_isCompat = isCompat;
}

NODISCARD std::string getHighestReportableVersionString()
{
    return g_versionString;
}

void setHighestReportableVersionString(const std::string &versionString)
{
    g_versionString = versionString;
}

} // namespace OpenGLConfig
