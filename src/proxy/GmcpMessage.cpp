// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpMessage.h"

#include "../global/CaseUtils.h"
#include "../global/Consts.h"
#include "../global/TextUtils.h"
#include "GmcpModule.h"
#include "GmcpUtils.h"

#include <sstream>

// REVISIT: use cached boxed type to avoid some allocations here?
NODISCARD static GmcpMessageName toGmcpMessageName(const GmcpMessageTypeEnum type)
{
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    case GmcpMessageTypeEnum::UPPER_CASE: \
        return GmcpMessageName{friendly};

    switch (type) {
        XFOREACH_GMCP_MESSAGE_TYPE(X_CASE)

    case GmcpMessageTypeEnum::UNKNOWN:
        // REVISIT: Do we really want this valid value to crash MMapper?
        break;
    }

#undef X_CASE
    abort();
}

NODISCARD static GmcpMessageTypeEnum toGmcpMessageType(const std::string_view str)
{
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    if (areEqualAsLowerUtf8(str, (normalized))) { \
        return GmcpMessageTypeEnum::UPPER_CASE; \
    }

    XFOREACH_GMCP_MESSAGE_TYPE(X_CASE)

#undef X_CASE
    return GmcpMessageTypeEnum::UNKNOWN;
}

// REVISIT: Why even bother storing the string version of the enum?
GmcpMessage::GmcpMessage(const GmcpMessageTypeEnum type)
    : m_name(toGmcpMessageName(type))
    , m_type(type)
{}

GmcpMessage::GmcpMessage(GmcpMessageName moved_package)
    : m_name{std::move(moved_package)}
    , m_type{toGmcpMessageType(m_name.getStdStringUtf8())}
{}

GmcpMessage::GmcpMessage(GmcpMessageName moved_package, GmcpJson moved_json)
    : m_name{std::move(moved_package)}
    , m_json{std::move(moved_json)}
    , m_document(GmcpJsonDocument::fromJson(m_json->toQByteArray()))
    , m_type{toGmcpMessageType(m_name.getStdStringUtf8())}
{}

GmcpMessage::GmcpMessage(const GmcpMessageTypeEnum type, GmcpJson moved_json)
    : GmcpMessage{toGmcpMessageName(type), std::move(moved_json)}
{}

QByteArray GmcpMessage::toRawBytes() const
{
    std::ostringstream oss;
    oss << m_name.getStdStringUtf8();
    if (m_json) {
        oss << char_consts::C_SPACE << m_json->getStdStringUtf8();
    }
    return mmqt::toQByteArrayUtf8(std::move(oss).str());
}

GmcpMessage GmcpMessage::fromRawBytes(const QByteArray &ba)
{
    const int pos = ba.indexOf(char_consts::C_SPACE);
    // <data> is optional
    if (pos == -1) {
        return GmcpMessage{GmcpMessageName{mmqt::toStdStringUtf8(ba)}};
    }
    auto package = GmcpMessageName{mmqt::toStdStringUtf8(ba.mid(0, pos))};
    auto json = GmcpJson{mmqt::toStdStringUtf8(ba.mid(pos + 1))};
    return GmcpMessage(std::move(package), std::move(json));
}
