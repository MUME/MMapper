// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpModule.h"

#include "../global/CaseUtils.h"
#include "../global/Consts.h"
#include "../global/TextUtils.h"

#include <exception>
#include <sstream>
#include <string>

NODISCARD static GmcpModuleTypeEnum toGmcpModuleType(const std::string_view str)
{
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    if (str == (normalized)) { \
        return GmcpModuleTypeEnum::UPPER_CASE; \
    }

    XFOREACH_GMCP_MODULE_TYPE(X_CASE)
#undef X_CASE

    return GmcpModuleTypeEnum::UNKNOWN;
}

GmcpModule::NameVersion GmcpModule::NameVersion::fromStdString(std::string moved_moduleVersion)
{
    const auto found = moved_moduleVersion.find(char_consts::C_SPACE);
    if (found == std::string::npos) {
        return NameVersion{::toLowerLatin1(std::move(moved_moduleVersion))};
    }

    const auto version = static_cast<uint32_t>(
        utils::clampNonNegative(std::stoi(moved_moduleVersion.substr(found + 1))));

    return NameVersion{::toLowerLatin1(moved_moduleVersion.substr(0, found)),
                       GmcpModuleVersion{version}};
}

GmcpModule::GmcpModule(std::string moved_moduleVersion)
    : m_nameVersion(NameVersion::fromStdString(std::move(moved_moduleVersion)))
    , m_type(toGmcpModuleType(m_nameVersion.normalizedName))
{}

GmcpModule::GmcpModule(const std::string &mod, const GmcpModuleVersion version)
{
    m_nameVersion.normalizedName = ::toLowerLatin1(mod);
    m_nameVersion.version = version;
    m_type = toGmcpModuleType(m_nameVersion.normalizedName);
}

GmcpModule::GmcpModule(const GmcpModuleTypeEnum type, const GmcpModuleVersion version)
    : m_type{type}
{
    m_nameVersion.version = version;
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    case GmcpModuleTypeEnum::UPPER_CASE: \
        m_nameVersion.normalizedName = normalized; \
        break;
    switch (type) {
        XFOREACH_GMCP_MODULE_TYPE(X_CASE)
    case GmcpModuleTypeEnum::UNKNOWN:
        abort();
    }
#undef X_CASE
}

std::string GmcpModule::toStdString() const
{
    std::ostringstream oss;
    oss << m_nameVersion.normalizedName;
    if (hasVersion()) {
        oss << char_consts::C_SPACE << m_nameVersion.version.asUint32();
    }
    return std::move(oss).str();
}
