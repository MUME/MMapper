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
struct EncodedLatin1;
/// Without entities
struct NODISCARD DecodedUnicode : QString
{
    using QString::QString;
    explicit DecodedUnicode(const char *s)
        : QString{QString::fromLatin1(s)}
    {}
    explicit DecodedUnicode(QString s)
        : QString{std::move(s)}
    {}

    DecodedUnicode(QByteArray) = delete;
    DecodedUnicode(QByteArray &&) = delete;
    DecodedUnicode(const QByteArray &) = delete;
    DecodedUnicode(EncodedLatin1) = delete;
    DecodedUnicode(EncodedLatin1 &&) = delete;
    DecodedUnicode(const EncodedLatin1 &) = delete;
};
/// With entities
struct NODISCARD EncodedLatin1 : QByteArray
{
    using QByteArray::QByteArray;
    explicit EncodedLatin1(const char *s)
        : QByteArray{s}
    {}
    explicit EncodedLatin1(QByteArray s)
        : QByteArray{std::move(s)}
    {}
    EncodedLatin1(QString) = delete;
    EncodedLatin1(QString &&) = delete;
    EncodedLatin1(const QString &) = delete;
    EncodedLatin1(DecodedUnicode) = delete;
    EncodedLatin1(DecodedUnicode &&) = delete;
    EncodedLatin1(const DecodedUnicode &) = delete;
};

enum class NODISCARD EncodingEnum { Translit, Lossless };
NODISCARD extern EncodedLatin1 encode(const DecodedUnicode &name,
                                      EncodingEnum encodingType = EncodingEnum::Translit);
NODISCARD extern DecodedUnicode decode(const EncodedLatin1 &input);

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
void foreachEntity(const QStringView line, EntityCallback &callback);

} // namespace entities
