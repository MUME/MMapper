// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "OpenGLConfig.h"

namespace OpenGLConfig {

static OpenGLProber::BackendType g_backendType;
static bool g_isCompat = false;
static std::string g_glVersionString;
static std::string g_esVersionString;
static int g_maxSamples = 0;

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

NODISCARD std::string getGLVersionString()
{
    return g_glVersionString;
}

void setGLVersionString(const std::string &versionString)
{
    g_glVersionString = versionString;
}

NODISCARD std::string getESVersionString()
{
    return g_esVersionString;
}

void setESVersionString(const std::string &versionString)
{
    g_esVersionString = versionString;
}

NODISCARD int getMaxSamples()
{
    return g_maxSamples;
}

void setMaxSamples(int maxSamples)
{
    g_maxSamples = maxSamples;
}

} // namespace OpenGLConfig
