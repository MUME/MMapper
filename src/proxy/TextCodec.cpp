// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "TextCodec.h"

#include "../configuration/configuration.h"
#include "../parser/parserutils.h"

#include <cassert>
#include <QDebug>

TextCodec::TextCodec(const TextCodecStrategyEnum textCodecStrategy)
    : textCodecStrategy{textCodecStrategy}
{
    switch (textCodecStrategy) {
    case TextCodecStrategyEnum::AUTO_SELECT_CODEC:
        setEncoding(getConfig().general.characterEncoding);
        break;
    case TextCodecStrategyEnum::FORCE_US_ASCII:
        setEncoding(CharacterEncodingEnum::ASCII);
        break;
    case TextCodecStrategyEnum::FORCE_LATIN_1:
        setEncoding(CharacterEncodingEnum::LATIN1);
        break;
    case TextCodecStrategyEnum::FORCE_UTF_8:
        setEncoding(CharacterEncodingEnum::UTF8);
        break;
    }
}

QStringList TextCodec::supportedEncodings() const
{
    switch (textCodecStrategy) {
    case TextCodecStrategyEnum::AUTO_SELECT_CODEC:
        return QStringList() << LATIN_1_ENCODING << UTF_8_ENCODING << US_ASCII_ENCODING;
    case TextCodecStrategyEnum::FORCE_LATIN_1:
        return QStringList() << LATIN_1_ENCODING;
    case TextCodecStrategyEnum::FORCE_UTF_8:
        return QStringList() << UTF_8_ENCODING;
    default:
    case TextCodecStrategyEnum::FORCE_US_ASCII:
        return QStringList() << US_ASCII_ENCODING;
    }
}

bool TextCodec::supports(const QByteArray &hisCharacterSet) const
{
    for (const auto &myCharacterSet : supportedEncodings()) {
        if (QString::compare(myCharacterSet, hisCharacterSet, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QByteArray TextCodec::fromUnicode(const QString &data)
{
    if (currentEncoding == CharacterEncodingEnum::ASCII) {
        // Simplify Latin-1 characters to US-ASCII
        QString outdata = data;
        ParserUtils::toAsciiInPlace(outdata);
        return textCodec->fromUnicode(outdata);
    }

    return textCodec->fromUnicode(data);
}

QString TextCodec::toUnicode(const QByteArray &data)
{
    return textCodec->toUnicode(data);
}

void TextCodec::setEncoding(const CharacterEncodingEnum encoding)
{
    switch (encoding) {
    case CharacterEncodingEnum::UTF8:
        textCodec = QTextCodec::codecForName(UTF_8_ENCODING);
        break;
    default:
    case CharacterEncodingEnum::ASCII:
    case CharacterEncodingEnum::LATIN1:
        textCodec = QTextCodec::codecForName(LATIN_1_ENCODING);
        break;
    }
    assert(textCodec);
    currentEncoding = encoding;
    qDebug() << "Switching to" << textCodec->name() << "character encoding";
}

void TextCodec::setEncodingForName(const QByteArray &encoding)
{
    if (QString::compare(encoding, textCodec->name(), Qt::CaseInsensitive) == 0) {
        // No need to switch encoding if its already set
        return;
    }

    if (textCodecStrategy != TextCodecStrategyEnum::AUTO_SELECT_CODEC) {
        qWarning() << "Could not switch change encoding to" << encoding << "with strategy"
                   << static_cast<int>(textCodecStrategy);
        return;
    }

    if (QString::compare(encoding, US_ASCII_ENCODING, Qt::CaseInsensitive) == 0) {
        setEncoding(CharacterEncodingEnum::ASCII);

    } else if (QString::compare(encoding, LATIN_1_ENCODING, Qt::CaseInsensitive) == 0) {
        setEncoding(CharacterEncodingEnum::LATIN1);

    } else if (QString::compare(encoding, UTF_8_ENCODING, Qt::CaseInsensitive) == 0) {
        setEncoding(CharacterEncodingEnum::UTF8);

    } else {
        qWarning() << "Unable to switch to unsupported codec" << encoding;
    }
}
