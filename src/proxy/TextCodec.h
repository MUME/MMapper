#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../configuration/configuration.h"

#include <QTextCodec>

class QTextCodec;

// supported IANA character sets
static constexpr const char *const LATIN_1_ENCODING = "ISO-8859-1";
static constexpr const char *const UTF_8_ENCODING = "UTF-8";
static constexpr const char *const US_ASCII_ENCODING = "US-ASCII";

enum class TextCodecStrategy { FORCE_US_ASCII, FORCE_LATIN_1, FORCE_UTF_8, AUTO_SELECT_CODEC };

class TextCodec
{
public:
    explicit TextCodec(TextCodecStrategy textCodecStrategy = TextCodecStrategy::AUTO_SELECT_CODEC);

    CharacterEncoding getEncoding() const { return currentEncoding; }
    void setEncoding(CharacterEncoding encoding);

    QByteArray fromUnicode(const QString &);
    QString toUnicode(const QByteArray &);

    void setEncodingForName(const QByteArray &encodingName);
    bool supports(const QByteArray &encodingName) const;
    QStringList supportedEncodings() const;

private:
    CharacterEncoding currentEncoding;
    QTextCodec *textCodec = nullptr;
    TextCodecStrategy textCodecStrategy{TextCodecStrategy::AUTO_SELECT_CODEC};
};
