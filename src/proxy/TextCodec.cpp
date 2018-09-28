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

#include <QDebug>

TextCodec::TextCodec(TextCodecStrategy textCodecStrategy)
    : textCodecStrategy{textCodecStrategy}
{
    const auto charset = getConfig().general.characterEncoding;

    switch (textCodecStrategy) {
    case TextCodecStrategy::AUTO_SELECT_CODEC:
        switch (charset) {
        case CharacterEncoding::ASCII:
            textCodec = QTextCodec::codecForName(US_ASCII_ENCODING);
            break;
        case CharacterEncoding::LATIN1:
            textCodec = QTextCodec::codecForName(LATIN_1_ENCODING);
            break;
        case CharacterEncoding::UTF8:
            textCodec = QTextCodec::codecForName(UTF_8_ENCODING);
            break;
        };
        break;
    case TextCodecStrategy::FORCE_US_ASCII:
        textCodec = QTextCodec::codecForName(US_ASCII_ENCODING);
        break;
    case TextCodecStrategy::FORCE_LATIN_1:
        textCodec = QTextCodec::codecForName(LATIN_1_ENCODING);
        break;
    case TextCodecStrategy::FORCE_UTF_8:
        textCodec = QTextCodec::codecForName(UTF_8_ENCODING);
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
    for (auto myCharacterSet : supportedEncodings()) {
        if (QString::compare(myCharacterSet, hisCharacterSet, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QByteArray TextCodec::fromUnicode(const QString &data)
{
    // Simplify Latin-1 character to US-ASCII because fromUnicode() does not do this
    if (textCodec->name() == US_ASCII_ENCODING) {
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

void TextCodec::setupEncoding(const QByteArray &encoding)
{
    if (QString::compare(encoding, textCodec->name(), Qt::CaseInsensitive) == 0) {
        // No need to switch encoding if its already set
        return;
    }

    if (textCodecStrategy != TextCodecStrategy::AUTO_SELECT_CODEC) {
        qWarning() << "Could not switch change encoding to" << encoding << "with strategy"
                   << static_cast<int>(textCodecStrategy);
        abort();
    }

    QTextCodec *candidateTextCodec = QTextCodec::codecForName(encoding);
    if (candidateTextCodec != nullptr) {
        textCodec = candidateTextCodec;
        qDebug() << "Switching to" << textCodec->name() << "character encoding";
    } else {
        qWarning() << "Unable to switch to codec" << encoding;
    }
}
