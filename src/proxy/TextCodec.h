#pragma once
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

#ifndef TEXTCODEC_H
#define TEXTCODEC_H

#include <QTextCodec>

class QTextCodec;

//supported IANA character sets
static constexpr const char *const LATIN_1_ENCODING = "ISO-8859-1";
static constexpr const char *const UTF_8_ENCODING = "UTF-8";
static constexpr const char *const US_ASCII_ENCODING = "US-ASCII";

enum class TextCodecStrategy { AUTO_SELECT_CODEC = 0, FORCE_US_ASCII, FORCE_LATIN_1, FORCE_UTF_8 };

class TextCodec
{
public:
    explicit TextCodec(TextCodecStrategy textCodecStrategy = TextCodecStrategy::AUTO_SELECT_CODEC);
    void setupEncoding(const QByteArray &encoding);
    QByteArray fromUnicode(const QString &);
    QString toUnicode(const QByteArray &);

    bool supports(const QByteArray &encoding) const;
    QStringList supportedEncodings() const;

private:
    QTextCodec *textCodec = nullptr;
    TextCodecStrategy textCodecStrategy{TextCodecStrategy::AUTO_SELECT_CODEC};
};

#endif // TEXTCODEC_H
