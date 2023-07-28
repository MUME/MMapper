// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpMessage.h"

#include <exception>
#include <sstream>

#include "../global/TextUtils.h"
#include "GmcpModule.h"
#include "GmcpUtils.h"

NODISCARD static GmcpMessageName toGmcpMessageName(const GmcpMessageTypeEnum type)
{
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    do { \
    case GmcpMessageTypeEnum::UPPER_CASE: \
        return GmcpMessageName(friendly); \
    } while (false);
    switch (type) {
        X_FOREACH_GMCP_MESSAGE_TYPE(X_CASE)
    case GmcpMessageTypeEnum::UNKNOWN:
        abort();
    }
#undef X_CASE
    abort();
}

GmcpMessage::GmcpMessage(const GmcpMessageTypeEnum type)
    : name(toGmcpMessageName(type))
    , type(type)
{}

NODISCARD static GmcpMessageTypeEnum toGmcpMessageType(const std::string &str)
{
    const auto lower = ::toLowerLatin1(str);
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    do { \
        if (lower.compare(normalized) == 0) \
            return GmcpMessageTypeEnum::UPPER_CASE; \
    } while (false);
    X_FOREACH_GMCP_MESSAGE_TYPE(X_CASE)
#undef X_CASE
    return GmcpMessageTypeEnum::UNKNOWN;
}

GmcpMessage::GmcpMessage()
    : name("")
    , type(GmcpMessageTypeEnum::UNKNOWN)
{}

GmcpMessage::GmcpMessage(const std::string &package)
    : name(package)
    , type(toGmcpMessageType(package))
{}

GmcpMessage::GmcpMessage(const std::string &package, const std::string &json)
    : name(package)
    , json(GmcpJson{json})
    , document(GmcpJsonDocument::fromJson(::toQByteArrayUtf8(json)))
    , type(toGmcpMessageType(package))
{}

GmcpMessage::GmcpMessage(const GmcpMessageTypeEnum type, const QString &json)
    : name(toGmcpMessageName(type))
    , json(::toStdStringUtf8(json))
    , document(GmcpJsonDocument::fromJson(json.toUtf8()))
    , type(type)
{}

GmcpMessage::GmcpMessage(const GmcpMessageTypeEnum type, const std::string &json)
    : name(toGmcpMessageName(type))
    , json(GmcpJson{json})
    , document(GmcpJsonDocument::fromJson(::toQByteArrayUtf8(json)))
    , type(type)
{}

QByteArray GmcpMessage::toRawBytes() const
{
    std::ostringstream oss;
    oss << name.getStdString();
    if (json)
        oss << ' ' << json->getStdString();
    return ::toQByteArrayUtf8(oss.str());
}

GmcpMessage GmcpMessage::fromRawBytes(const QByteArray &ba)
{
    const auto pos = ba.indexOf(' ');
    // <data> is optional
    if (pos == -1)
        return GmcpMessage(ba.toStdString());

    const std::string package = ba.mid(0, pos).toStdString(); // Latin-1
    const std::string json = ba.mid(pos + 1).toStdString();   // UTF-8
    return GmcpMessage(package, json);
}
