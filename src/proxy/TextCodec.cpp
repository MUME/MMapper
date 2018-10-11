/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "TextCodec.h"

#include "../configuration/configuration.h"
#include "../parser/parserutils.h"

#include <cassert>
#include <QDebug>

TextCodec::TextCodec(TextCodecStrategy textCodecStrategy)
    : textCodecStrategy{textCodecStrategy}
{
    switch (textCodecStrategy) {
    case TextCodecStrategy::AUTO_SELECT_CODEC:
        setEncoding(getConfig().general.characterEncoding);
        break;
    case TextCodecStrategy::FORCE_US_ASCII:
        setEncoding(CharacterEncoding::ASCII);
        break;
    case TextCodecStrategy::FORCE_LATIN_1:
        setEncoding(CharacterEncoding::LATIN1);
        break;
    case TextCodecStrategy::FORCE_UTF_8:
        setEncoding(CharacterEncoding::UTF8);
        break;
    };
}

QStringList TextCodec::supportedEncodings() const
{
    switch (textCodecStrategy) {
    case TextCodecStrategy::AUTO_SELECT_CODEC:
        return QStringList() << LATIN_1_ENCODING << UTF_8_ENCODING << US_ASCII_ENCODING;
    case TextCodecStrategy::FORCE_LATIN_1:
        return QStringList() << LATIN_1_ENCODING;
    case TextCodecStrategy::FORCE_UTF_8:
        return QStringList() << UTF_8_ENCODING;
    default:
    case TextCodecStrategy::FORCE_US_ASCII:
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
    if (currentEncoding == CharacterEncoding::ASCII) {
        // Simplify Latin-1 characters to US-ASCII
        QString outdata = data;
        ParserUtils::latinToAsciiInPlace(outdata);
        return textCodec->fromUnicode(outdata);
    }

    return textCodec->fromUnicode(data);
}

QString TextCodec::toUnicode(const QByteArray &data)
{
    return textCodec->toUnicode(data);
}

void TextCodec::setEncoding(CharacterEncoding encoding)
{
    switch (encoding) {
    case CharacterEncoding::UTF8:
        textCodec = QTextCodec::codecForName(UTF_8_ENCODING);
        break;
    default:
    case CharacterEncoding::ASCII:
    case CharacterEncoding::LATIN1:
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

    if (textCodecStrategy != TextCodecStrategy::AUTO_SELECT_CODEC) {
        qWarning() << "Could not switch change encoding to" << encoding << "with strategy"
                   << static_cast<int>(textCodecStrategy);
        return;
    }

    if (QString::compare(encoding, US_ASCII_ENCODING, Qt::CaseInsensitive) == 0) {
        setEncoding(CharacterEncoding::ASCII);

    } else if (QString::compare(encoding, LATIN_1_ENCODING, Qt::CaseInsensitive) == 0) {
        setEncoding(CharacterEncoding::LATIN1);

    } else if (QString::compare(encoding, UTF_8_ENCODING, Qt::CaseInsensitive) == 0) {
        setEncoding(CharacterEncoding::UTF8);

    } else {
        qWarning() << "Unable to switch to unsupported codec" << encoding;
    }
}
