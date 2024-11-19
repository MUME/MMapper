#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"

#include <functional>
#include <optional>

#include <QByteArray>
#include <QString>

static constexpr const uint32_t MAX_UNICODE_CODEPOINT = 0x10FFFFu;

using OptQChar = std::optional<const QChar>;

namespace entities {
struct EncodedString;
/// Without entities
struct NODISCARD DecodedString : QString
{
    using QString::QString;
    explicit DecodedString(const char *s)
        : QString{QString::fromUtf8(s)}
    {}
    explicit DecodedString(QString s)
        : QString{std::move(s)}
    {}

    DecodedString(QByteArray) = delete;
    DecodedString(QByteArray &&) = delete;
    DecodedString(const QByteArray &) = delete;
    DecodedString(EncodedString) = delete;
    DecodedString(EncodedString &&) = delete;
    DecodedString(const EncodedString &) = delete;
};
/// With entities
struct NODISCARD EncodedString : QString
{
    using QString::QString;
    explicit EncodedString(const char *s)
        : QString{s}
    {}
    explicit EncodedString(QString s)
        : QString{std::move(s)}
    {}
    EncodedString(QByteArray) = delete;
    EncodedString(QByteArray &&) = delete;
    EncodedString(const QByteArray &) = delete;
    EncodedString(DecodedString) = delete;
    EncodedString(DecodedString &&) = delete;
    EncodedString(const DecodedString &) = delete;
};

enum class NODISCARD EncodingEnum { Translit, Lossless };
NODISCARD extern EncodedString encode(const DecodedString &name,
                                      EncodingEnum encodingType = EncodingEnum::Translit);
NODISCARD extern DecodedString decode(const EncodedString &input);

struct NODISCARD EntityCallback
{
public:
    virtual ~EntityCallback();

private:
    virtual void virt_decodedEntity(int start, int len, OptQChar decoded) = 0;

public:
    void decodedEntity(const int start, const int len, const OptQChar decoded)
    {
        virt_decodedEntity(start, len, decoded);
    }
};
void foreachEntity(QStringView line, EntityCallback &callback);

} // namespace entities

namespace test {
extern void test_entities();
} // namespace test
