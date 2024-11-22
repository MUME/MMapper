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
        X_FOREACH_GMCP_MESSAGE_TYPE(X_CASE)

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
    if (areEqualAsLowerLatin1(str, (normalized))) \
        return GmcpMessageTypeEnum::UPPER_CASE;

    X_FOREACH_GMCP_MESSAGE_TYPE(X_CASE)

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
    , m_type{toGmcpMessageType(m_name.getStdString())}
{}

GmcpMessage::GmcpMessage(GmcpMessageName moved_package, GmcpJson moved_json)
    : m_name{std::move(moved_package)}
    , m_json{std::move(moved_json)}
    , m_document(GmcpJsonDocument::fromJson(m_json->toQByteArray()))
    , m_type{toGmcpMessageType(m_name.getStdString())}
{}

GmcpMessage::GmcpMessage(const GmcpMessageTypeEnum type, GmcpJson moved_json)
    : GmcpMessage{toGmcpMessageName(type), std::move(moved_json)}
{}

QByteArray GmcpMessage::toRawBytes() const
{
    // FIXME: Mixing Latin1 and UTF8 is asking for trouble.
    std::ostringstream oss;
    oss << m_name.getStdString();
    if (m_json)
        oss << char_consts::C_SPACE << m_json->getStdString();
    return mmqt::toQByteArrayUtf8(std::move(oss).str());
}

GmcpMessage GmcpMessage::fromRawBytes(const QByteArray &ba)
{
    // FIXME: Mixing Latin1 and UTF8 is asking for trouble.
    // Throw if the message name isn't ASCII?

    const int pos = ba.indexOf(char_consts::C_SPACE);
    // <data> is optional
    if (pos == -1)
        return GmcpMessage{GmcpMessageName{mmqt::toStdStringLatin1(ba)}};

    auto package = GmcpMessageName{mmqt::toStdStringLatin1(ba.mid(0, pos))}; // Latin-1
    auto json = GmcpJson{mmqt::toStdStringUtf8(ba.mid(pos + 1))};            // UTF-8
    return GmcpMessage(std::move(package), std::move(json));
}
