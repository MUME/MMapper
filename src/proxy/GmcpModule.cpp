// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpModule.h"

#include <exception>
#include <sstream>
#include <string>
#include <QString>

#include "../global/TextUtils.h"

GmcpModule::GmcpModule(const QString &moduleVersion)
    : GmcpModule(::toStdStringLatin1(moduleVersion))
{}

NODISCARD static GmcpModuleTypeEnum toGmcpModuleType(const std::string &str)
{
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    do { \
        if (str.compare(normalized) == 0) \
            return GmcpModuleTypeEnum::UPPER_CASE; \
    } while (false);
    X_FOREACH_GMCP_MODULE_TYPE(X_CASE)
#undef X_CASE
    return GmcpModuleTypeEnum::UNKNOWN;
}

GmcpModule::GmcpModule(const std::string &moduleVersion)
{
    auto found = moduleVersion.find(' ');
    if (found == std::string::npos) {
        normalizedName = ::toLowerLatin1(moduleVersion);
    } else {
        normalizedName = ::toLowerLatin1(moduleVersion.substr(0, found));
        const auto stoi = std::stoi(moduleVersion.substr(found + 1));
        version = GmcpModuleVersion(static_cast<uint32_t>(utils::clampNonNegative(stoi)));
    }
    type = toGmcpModuleType(normalizedName);
}

GmcpModule::GmcpModule(const std::string &module, const GmcpModuleVersion version)
{
    normalizedName = ::toLowerLatin1(module);
    this->version = version;
    type = toGmcpModuleType(normalizedName);
}

GmcpModule::GmcpModule(const GmcpModuleTypeEnum type, const GmcpModuleVersion version)
{
    this->type = type;
    this->version = version;
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    case GmcpModuleTypeEnum::UPPER_CASE: \
        normalizedName = normalized; \
        break;
    switch (type) {
        X_FOREACH_GMCP_MODULE_TYPE(X_CASE)
    case GmcpModuleTypeEnum::UNKNOWN:
        abort();
    }
#undef X_CASE
}

std::string GmcpModule::toStdString() const
{
    std::ostringstream oss;
    oss << normalizedName;
    if (hasVersion())
        oss << ' ' << version.asUint32();
    return oss.str();
}
